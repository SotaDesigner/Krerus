#include "CodexBlueprintBridgeModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputKey.h"
#include "K2Node_EnhancedInputAction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "PlayInEditorDataTypes.h"
#include "PackageTools.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Logging/TokenizedMessage.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"
#include "AutomatedAssetImportData.h"
#include "HighResScreenshot.h"
#include "WidgetBlueprintFactory.h"
#include "K2Node.h"

DEFINE_LOG_CATEGORY_STATIC(LogCodexBlueprintBridge, Log, All);

namespace
{
TUniquePtr<FHttpServerResponse> JsonResponse(
    const TSharedRef<FJsonObject>& Object,
    EHttpServerResponseCodes Code = EHttpServerResponseCodes::Ok)
{
    FString Body;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    FJsonSerializer::Serialize(Object, Writer);
    TUniquePtr<FHttpServerResponse> Response =
        FHttpServerResponse::Create(Body, TEXT("application/json; charset=utf-8"));
    Response->Code = Code;
    Response->Headers.Add(TEXT("Cache-Control"), {TEXT("no-store")});
    return Response;
}

TUniquePtr<FHttpServerResponse> ErrorResponse(
    const FString& Message,
    EHttpServerResponseCodes Code)
{
    const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetBoolField(TEXT("ok"), false);
    Object->SetStringField(TEXT("error"), Message);
    return JsonResponse(Object, Code);
}

FString QueryValue(const FHttpServerRequest& Request, const FString& Name)
{
    const FString* Value = Request.QueryParams.Find(Name);
    return Value ? *Value : FString();
}

bool ParseJsonBody(
    const FHttpServerRequest& Request,
    TSharedPtr<FJsonObject>& OutObject,
    FString& OutError)
{
    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
    const FString Body(Converted.Length(), Converted.Get());
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
    if (!FJsonSerializer::Deserialize(Reader, OutObject) || !OutObject.IsValid())
    {
        OutError = TEXT("Request body must be a JSON object");
        return false;
    }
    return true;
}

bool SaveBlueprint(UBlueprint* Blueprint, FString& OutError)
{
    if (!Blueprint || !Blueprint->GetOutermost())
    {
        OutError = TEXT("Blueprint package is unavailable");
        return false;
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        OutError = TEXT("Blueprint compilation failed; asset was not saved");
        return false;
    }

    UPackage* Package = Blueprint->GetOutermost();
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
    {
        OutError = TEXT("Unreal could not save the Blueprint package");
        return false;
    }
    return true;
}

bool PinTypeFromName(const FString& TypeName, FEdGraphPinType& OutType)
{
    if (TypeName.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (TypeName.Equals(TEXT("integer"), ESearchCase::IgnoreCase)
        || TypeName.Equals(TEXT("int"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (TypeName.Equals(TEXT("float"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Real;
        OutType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (TypeName.Equals(TEXT("string"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (TypeName.Equals(TEXT("name"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Name;
    }
    else if (TypeName.Equals(TEXT("text"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }
    else if (TypeName.Equals(TEXT("byte"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Byte;
    }
    else if (TypeName.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else if (TypeName.Equals(TEXT("rotator"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
    }
    else if (TypeName.Equals(TEXT("transform"), ESearchCase::IgnoreCase))
    {
        OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
    }
    else
    {
        return false;
    }
    return true;
}

bool ResolvePropertyPath(
    UObject* Root,
    const FString& RawPath,
    UObject*& OutOwner,
    FProperty*& OutProperty,
    void*& OutValue,
    FString& OutError)
{
    FString Path = RawPath;
    Path.RemoveFromStart(TEXT("ClassDefaults."), ESearchCase::IgnoreCase);
    TArray<FString> Segments;
    Path.ParseIntoArray(Segments, TEXT("."), true);
    if (!Root || Segments.Num() == 0)
    {
        OutError = TEXT("property_path is empty");
        return false;
    }

    UObject* CurrentObject = Root;
    UStruct* CurrentStruct = Root->GetClass();
    void* CurrentMemory = Root;
    for (int32 Index = 0; Index < Segments.Num(); ++Index)
    {
        FProperty* Property = FindFProperty<FProperty>(CurrentStruct, FName(*Segments[Index]));
        if (!Property)
        {
            OutError = FString::Printf(TEXT("Property '%s' was not found on %s"),
                *Segments[Index], *CurrentStruct->GetName());
            return false;
        }
        void* Value = Property->ContainerPtrToValuePtr<void>(CurrentMemory);
        if (Index == Segments.Num() - 1)
        {
            OutOwner = CurrentObject;
            OutProperty = Property;
            OutValue = Value;
            return true;
        }
        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            CurrentObject = ObjectProperty->GetObjectPropertyValue(Value);
            if (!CurrentObject)
            {
                OutError = TEXT("Intermediate object property is null: ") + Segments[Index];
                return false;
            }
            CurrentStruct = CurrentObject->GetClass();
            CurrentMemory = CurrentObject;
        }
        else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            CurrentStruct = StructProperty->Struct;
            CurrentMemory = Value;
        }
        else
        {
            OutError = TEXT("Intermediate property is not an object or struct: ") + Segments[Index];
            return false;
        }
    }
    return false;
}

TSharedRef<FJsonObject> ReflectedPropertyToJson(UObject* Owner, FProperty* Property)
{
    const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
    Item->SetStringField(TEXT("name"), Property->GetName());
    Item->SetStringField(TEXT("type"), Property->GetCPPType());
    Item->SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
    Item->SetBoolField(TEXT("editable"), Property->HasAnyPropertyFlags(CPF_Edit));
    FString Exported;
    Property->ExportTextItem_Direct(
        Exported,
        Property->ContainerPtrToValuePtr<void>(Owner),
        nullptr,
        Owner,
        PPF_None);
    Item->SetStringField(TEXT("value"), Exported);
    return Item;
}

UEdGraph* FindBlueprintGraph(UBlueprint* Blueprint, const FString& GraphName)
{
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);
    for (UEdGraph* Graph : AllGraphs)
    {
        if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
        {
            return Graph;
        }
    }
    return nullptr;
}

UFunction* ResolveFunction(const FString& FunctionPath)
{
    if (UFunction* Function = FindObject<UFunction>(nullptr, *FunctionPath))
    {
        return Function;
    }
    FString OwnerPath;
    FString FunctionName;
    if (FunctionPath.Split(TEXT(":"), &OwnerPath, &FunctionName,
        ESearchCase::CaseSensitive, ESearchDir::FromEnd))
    {
        if (UClass* OwnerClass = LoadObject<UClass>(nullptr, *OwnerPath))
        {
            return OwnerClass->FindFunctionByName(FName(*FunctionName));
        }
    }
    return nullptr;
}

TSharedRef<FJsonObject> PinToJson(const UEdGraphPin* Pin)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Pin->PinId.ToString());
    Result->SetStringField(TEXT("name"), Pin->PinName.ToString());
    Result->SetStringField(TEXT("direction"),
        Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
    Result->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
    Result->SetStringField(TEXT("subcategory"), Pin->PinType.PinSubCategory.ToString());
    Result->SetStringField(TEXT("default_value"), Pin->DefaultValue);

    TArray<TSharedPtr<FJsonValue>> Links;
    for (const UEdGraphPin* Linked : Pin->LinkedTo)
    {
        if (Linked && Linked->GetOwningNode())
        {
            const FString Link = Linked->GetOwningNode()->NodeGuid.ToString()
                + TEXT(":") + Linked->PinId.ToString();
            Links.Add(MakeShared<FJsonValueString>(Link));
        }
    }
    Result->SetArrayField(TEXT("links"), Links);
    return Result;
}

TSharedRef<FJsonObject> GraphToJson(const UEdGraph* Graph, const FString& Kind)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), Graph->GetName());
    Result->SetStringField(TEXT("kind"), Kind);

    TArray<TSharedPtr<FJsonValue>> Nodes;
    for (const UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        const TSharedRef<FJsonObject> NodeJson = MakeShared<FJsonObject>();
        NodeJson->SetStringField(TEXT("id"), Node->NodeGuid.ToString());
        NodeJson->SetStringField(TEXT("class"), Node->GetClass()->GetName());
        NodeJson->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
        NodeJson->SetNumberField(TEXT("x"), Node->NodePosX);
        NodeJson->SetNumberField(TEXT("y"), Node->NodePosY);

        TArray<TSharedPtr<FJsonValue>> Pins;
        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin)
            {
                Pins.Add(MakeShared<FJsonValueObject>(PinToJson(Pin)));
            }
        }
        NodeJson->SetArrayField(TEXT("pins"), Pins);
        Nodes.Add(MakeShared<FJsonValueObject>(NodeJson));
    }
    Result->SetArrayField(TEXT("nodes"), Nodes);
    return Result;
}

template <typename GraphArrayType>
void AppendGraphs(
    TArray<TSharedPtr<FJsonValue>>& Output,
    const GraphArrayType& Graphs,
    const FString& Kind)
{
    for (const UEdGraph* Graph : Graphs)
    {
        if (Graph)
        {
            Output.Add(MakeShared<FJsonValueObject>(GraphToJson(Graph, Kind)));
        }
    }
}
}

void FCodexBlueprintBridgeModule::StartupModule()
{
    StartServer();
}

void FCodexBlueprintBridgeModule::ShutdownModule()
{
    StopServer();
}

void FCodexBlueprintBridgeModule::StartServer()
{
    FHttpServerModule& Server = FHttpServerModule::Get();
    Router = Server.GetHttpRouter(Port);
    if (!Router.IsValid())
    {
        UE_LOG(LogCodexBlueprintBridge, Error, TEXT("Could not create local HTTP router"));
        return;
    }

    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/health")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleHealth(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprints")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleListBlueprints(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleInspectBlueprint(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/edit-mode")),
        EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleEditMode(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/create")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleCreateBlueprint(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/add-variable")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleAddVariable(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/apply-graph-patch")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleApplyGraphPatch(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/class-defaults")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleClassDefaults(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/set-default-property")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleSetDefaultProperty(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/edit-graph")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleEditGraph(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/undo")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleUndo(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/enhanced-input")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleEnhancedInput(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/actions")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleSearchActions(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/action")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleDescribeAction(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(
        FHttpPath(TEXT("/v1/blueprint/compile")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleCompileBlueprint(Request, OnComplete);
        })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/assets/duplicate")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandleDuplicateAsset(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/function")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandleBlueprintFunction(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/dispatcher")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandleBlueprintDispatcher(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/interface")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandleBlueprintInterface(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/component")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandleBlueprintComponent(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/pie")), EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R, const FHttpResultCallback& C){ return HandlePIE(R, C); })));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/snapshot")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleSnapshot(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/region")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleInspectRegion(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/component-details")), EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleComponentDetails(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/analyze-extract-function")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleAnalyzeExtractFunction(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/widget-tree")), EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleWidgetTree(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/assets/import")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleImportAsset(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/assets")), EHttpServerRequestVerbs::VERB_GET,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleListAssets(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/pie/capture")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleCapturePIE(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/widget-blueprint/create")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleCreateWidgetBlueprint(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/variable-metadata")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleVariableMetadata(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/stale-pins")), EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleStalePins(R,C);})));
    RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/v1/blueprint/protected-compile")), EHttpServerRequestVerbs::VERB_POST,
        FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& R,const FHttpResultCallback& C){return HandleProtectedCompile(R,C);})));

    Server.StartAllListeners();
    UE_LOG(LogCodexBlueprintBridge, Display,
        TEXT("Read-only bridge listening on http://127.0.0.1:%u"), Port);
}

void FCodexBlueprintBridgeModule::StopServer()
{
    if (Router.IsValid())
    {
        for (const FHttpRouteHandle& Handle : RouteHandles)
        {
            Router->UnbindRoute(Handle);
        }
    }
    RouteHandles.Empty();
    Router.Reset();
}

bool FCodexBlueprintBridgeModule::HandleHealth(
    const FHttpServerRequest&,
    const FHttpResultCallback& OnComplete)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetStringField(TEXT("plugin"), TEXT("CodexBlueprintBridge"));
    Result->SetStringField(TEXT("version"), TEXT("0.8.0"));
    Result->SetStringField(TEXT("mode"), bEditModeEnabled ? TEXT("editing") : TEXT("read-only"));
    Result->SetBoolField(TEXT("edit_mode_enabled"), bEditModeEnabled);
    Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleListBlueprints(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    FString Search = QueryValue(Request, TEXT("search"));
    Search.TrimStartAndEndInline();
    int32 Limit = FCString::Atoi(*QueryValue(Request, TEXT("limit")));
    Limit = FMath::Clamp(Limit > 0 ? Limit : 100, 1, 1000);

    FAssetRegistryModule& RegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.PackagePaths.Add(FName(TEXT("/Game")));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    RegistryModule.Get().GetAssets(Filter, Assets);
    Assets.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.GetObjectPathString() < B.GetObjectPathString();
    });

    TArray<TSharedPtr<FJsonValue>> Items;
    for (const FAssetData& Asset : Assets)
    {
        const FString ObjectPath = Asset.GetObjectPathString();
        if (!Search.IsEmpty() && !ObjectPath.Contains(Search, ESearchCase::IgnoreCase))
        {
            continue;
        }

        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        Item->SetStringField(TEXT("path"), ObjectPath);
        Item->SetStringField(TEXT("package"), Asset.PackageName.ToString());
        Items.Add(MakeShared<FJsonValueObject>(Item));
        if (Items.Num() >= Limit)
        {
            break;
        }
    }

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetNumberField(TEXT("returned"), Items.Num());
    Result->SetNumberField(TEXT("total_blueprints"), Assets.Num());
    Result->SetArrayField(TEXT("blueprints"), Items);
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleInspectBlueprint(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    const FString Path = QueryValue(Request, TEXT("path"));
    if (Path.IsEmpty() || !Path.StartsWith(TEXT("/Game/")))
    {
        OnComplete(ErrorResponse(TEXT("A /Game object path is required"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
    if (!Blueprint)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint not found"),
            EHttpServerResponseCodes::NotFound));
        return true;
    }

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetStringField(TEXT("name"), Blueprint->GetName());
    Result->SetStringField(TEXT("path"), Blueprint->GetPathName());
    Result->SetStringField(TEXT("parent_class"),
        Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : FString());

    TArray<TSharedPtr<FJsonValue>> Variables;
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), Variable.VarName.ToString());
        Item->SetStringField(TEXT("category"), Variable.Category.ToString());
        Item->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        Variables.Add(MakeShared<FJsonValueObject>(Item));
    }
    Result->SetArrayField(TEXT("variables"), Variables);

    TArray<TSharedPtr<FJsonValue>> Graphs;
    AppendGraphs(Graphs, Blueprint->UbergraphPages, TEXT("event"));
    AppendGraphs(Graphs, Blueprint->FunctionGraphs, TEXT("function"));
    AppendGraphs(Graphs, Blueprint->MacroGraphs, TEXT("macro"));
    AppendGraphs(Graphs, Blueprint->DelegateSignatureGraphs, TEXT("delegate"));
    Result->SetArrayField(TEXT("graphs"), Graphs);

    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleEditMode(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    if (Request.Verb == EHttpServerRequestVerbs::VERB_POST)
    {
        TSharedPtr<FJsonObject> Body;
        FString ParseError;
        if (!ParseJsonBody(Request, Body, ParseError))
        {
            OnComplete(ErrorResponse(ParseError, EHttpServerResponseCodes::BadRequest));
            return true;
        }

        bool bEnable = false;
        if (!Body->TryGetBoolField(TEXT("enabled"), bEnable))
        {
            OnComplete(ErrorResponse(TEXT("Boolean field 'enabled' is required"),
                EHttpServerResponseCodes::BadRequest));
            return true;
        }

        if (bEnable)
        {
            FString Confirmation;
            Body->TryGetStringField(TEXT("confirmation"), Confirmation);
            if (Confirmation != TEXT("ENABLE_BLUEPRINT_EDITING"))
            {
                OnComplete(ErrorResponse(
                    TEXT("Use confirmation ENABLE_BLUEPRINT_EDITING to unlock this editor session"),
                    EHttpServerResponseCodes::Forbidden));
                return true;
            }
        }
        bEditModeEnabled = bEnable;
    }

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("edit_mode_enabled"), bEditModeEnabled);
    Result->SetStringField(TEXT("mode"), bEditModeEnabled ? TEXT("editing") : TEXT("read-only"));
    Result->SetStringField(TEXT("lock_scope"), TEXT("current Unreal Editor session"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleCreateBlueprint(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString ParseError;
    if (!ParseJsonBody(Request, Body, ParseError))
    {
        OnComplete(ErrorResponse(ParseError, EHttpServerResponseCodes::BadRequest));
        return true;
    }

    FString PackageName;
    Body->TryGetStringField(TEXT("package_name"), PackageName);
    FString ParentClassPath = TEXT("/Script/Engine.Actor");
    Body->TryGetStringField(TEXT("parent_class"), ParentClassPath);
    bool bDryRun = true;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);

    if (!PackageName.StartsWith(TEXT("/Game/"))
        || !FPackageName::IsValidLongPackageName(PackageName))
    {
        OnComplete(ErrorResponse(
            TEXT("package_name must be a valid /Game package path, for example /Game/Codex/BP_Test"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    if (FindObject<UObject>(nullptr, *(PackageName + TEXT(".")
        + FPackageName::GetLongPackageAssetName(PackageName))))
    {
        OnComplete(ErrorResponse(TEXT("An asset already exists at that path"),
            EHttpServerResponseCodes::Conflict));
        return true;
    }

    UClass* ParentClass = LoadObject<UClass>(nullptr, *ParentClassPath);
    if (!ParentClass || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
    {
        OnComplete(ErrorResponse(TEXT("parent_class cannot be used for a Blueprint"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }

    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetStringField(TEXT("operation"), TEXT("create_blueprint"));
        Preview->SetStringField(TEXT("package_name"), PackageName);
        Preview->SetStringField(TEXT("parent_class"), ParentClass->GetPathName());
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled)
    {
        OnComplete(ErrorResponse(TEXT("Edit mode is locked"),
            EHttpServerResponseCodes::Forbidden));
        return true;
    }

    const FScopedTransaction Transaction(NSLOCTEXT(
        "CodexBlueprintBridge", "CreateBlueprint", "Codex: Create Blueprint"));
    UPackage* Package = CreatePackage(*PackageName);
    const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        FName(*AssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName(TEXT("CodexBlueprintBridge")));
    if (!Blueprint)
    {
        OnComplete(ErrorResponse(TEXT("Unreal could not create the Blueprint"),
            EHttpServerResponseCodes::ServerError));
        return true;
    }
    FAssetRegistryModule::AssetCreated(Blueprint);

    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint = Blueprint;

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetStringField(TEXT("operation"), TEXT("create_blueprint"));
    Result->SetStringField(TEXT("path"), Blueprint->GetPathName());
    Result->SetStringField(TEXT("compile_status"), TEXT("success"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleAddVariable(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString ParseError;
    if (!ParseJsonBody(Request, Body, ParseError))
    {
        OnComplete(ErrorResponse(ParseError, EHttpServerResponseCodes::BadRequest));
        return true;
    }

    FString BlueprintPath;
    FString VariableName;
    FString TypeName;
    FString DefaultValue;
    Body->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);
    Body->TryGetStringField(TEXT("variable_name"), VariableName);
    Body->TryGetStringField(TEXT("variable_type"), TypeName);
    Body->TryGetStringField(TEXT("default_value"), DefaultValue);
    bool bDryRun = true;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);

    UBlueprint* Blueprint = BlueprintPath.StartsWith(TEXT("/Game/"))
        ? LoadObject<UBlueprint>(nullptr, *BlueprintPath) : nullptr;
    if (!Blueprint)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint not found"),
            EHttpServerResponseCodes::NotFound));
        return true;
    }
    if (VariableName.IsEmpty()
        || !FName(*VariableName).IsValidXName(INVALID_OBJECTNAME_CHARACTERS))
    {
        OnComplete(ErrorResponse(TEXT("variable_name is invalid"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VariableName)) != INDEX_NONE)
    {
        OnComplete(ErrorResponse(TEXT("The Blueprint already contains that variable"),
            EHttpServerResponseCodes::Conflict));
        return true;
    }

    FEdGraphPinType PinType;
    FString TypePath;
    Body->TryGetStringField(TEXT("type_path"), TypePath);
    bool bTypeValid = PinTypeFromName(TypeName, PinType);
    if (!bTypeValid && TypeName.Equals(TEXT("object"), ESearchCase::IgnoreCase))
    {
        if (UClass* Class = LoadObject<UClass>(nullptr, *TypePath))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            PinType.PinSubCategoryObject = Class;
            bTypeValid = true;
        }
    }
    else if (!bTypeValid && TypeName.Equals(TEXT("class"), ESearchCase::IgnoreCase))
    {
        if (UClass* Class = LoadObject<UClass>(nullptr, *TypePath))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
            PinType.PinSubCategoryObject = Class;
            bTypeValid = true;
        }
    }
    else if (!bTypeValid && TypeName.Equals(TEXT("struct"), ESearchCase::IgnoreCase))
    {
        if (UScriptStruct* Struct = LoadObject<UScriptStruct>(nullptr, *TypePath))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            PinType.PinSubCategoryObject = Struct;
            bTypeValid = true;
        }
    }
    else if (!bTypeValid && TypeName.Equals(TEXT("enum"), ESearchCase::IgnoreCase))
    {
        if (UEnum* Enum = LoadObject<UEnum>(nullptr, *TypePath))
        {
            PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
            PinType.PinSubCategoryObject = Enum;
            bTypeValid = true;
        }
    }
    if (!bTypeValid)
    {
        OnComplete(ErrorResponse(
            TEXT("Unsupported variable type or invalid type_path"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    FString Container;
    Body->TryGetStringField(TEXT("container"), Container);
    if (Container.Equals(TEXT("array"), ESearchCase::IgnoreCase))
    {
        PinType.ContainerType = EPinContainerType::Array;
    }
    else if (Container.Equals(TEXT("set"), ESearchCase::IgnoreCase))
    {
        PinType.ContainerType = EPinContainerType::Set;
    }
    else if (Container.Equals(TEXT("map"), ESearchCase::IgnoreCase))
    {
        FString MapValueTypeName;
        Body->TryGetStringField(TEXT("map_value_type"), MapValueTypeName);
        FEdGraphPinType MapValueType;
        if (!PinTypeFromName(MapValueTypeName, MapValueType))
        {
            OnComplete(ErrorResponse(TEXT("map_value_type is invalid"),
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
        PinType.ContainerType = EPinContainerType::Map;
        PinType.PinValueType.TerminalCategory = MapValueType.PinCategory;
        PinType.PinValueType.TerminalSubCategory = MapValueType.PinSubCategory;
        PinType.PinValueType.TerminalSubCategoryObject = MapValueType.PinSubCategoryObject;
    }

    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetStringField(TEXT("operation"), TEXT("add_variable"));
        Preview->SetStringField(TEXT("blueprint_path"), Blueprint->GetPathName());
        Preview->SetStringField(TEXT("variable_name"), VariableName);
        Preview->SetStringField(TEXT("variable_type"), TypeName);
        Preview->SetStringField(TEXT("default_value"), DefaultValue);
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled)
    {
        OnComplete(ErrorResponse(TEXT("Edit mode is locked"),
            EHttpServerResponseCodes::Forbidden));
        return true;
    }

    const FScopedTransaction Transaction(NSLOCTEXT(
        "CodexBlueprintBridge", "AddVariable", "Codex: Add Blueprint Variable"));
    Blueprint->Modify();
    if (!FBlueprintEditorUtils::AddMemberVariable(
        Blueprint, FName(*VariableName), PinType, DefaultValue))
    {
        OnComplete(ErrorResponse(TEXT("Unreal could not add the variable"),
            EHttpServerResponseCodes::ServerError));
        return true;
    }

    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint = Blueprint;

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetStringField(TEXT("operation"), TEXT("add_variable"));
    Result->SetStringField(TEXT("blueprint_path"), Blueprint->GetPathName());
    Result->SetStringField(TEXT("variable_name"), VariableName);
    Result->SetStringField(TEXT("compile_status"), TEXT("success"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleApplyGraphPatch(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString ParseError;
    if (!ParseJsonBody(Request, Body, ParseError))
    {
        OnComplete(ErrorResponse(ParseError, EHttpServerResponseCodes::BadRequest));
        return true;
    }

    FString BlueprintPath;
    FString GraphName;
    Body->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);
    Body->TryGetStringField(TEXT("graph_name"), GraphName);
    bool bDryRun = true;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);

    UBlueprint* Blueprint = BlueprintPath.StartsWith(TEXT("/Game/"))
        ? LoadObject<UBlueprint>(nullptr, *BlueprintPath) : nullptr;
    UEdGraph* Graph = Blueprint ? FindBlueprintGraph(Blueprint, GraphName) : nullptr;
    if (!Blueprint || !Graph)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint or graph not found"),
            EHttpServerResponseCodes::NotFound));
        return true;
    }

    const TArray<TSharedPtr<FJsonValue>>* NodeValues = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* ConnectionValues = nullptr;
    if (!Body->TryGetArrayField(TEXT("nodes"), NodeValues) || !NodeValues
        || NodeValues->Num() == 0)
    {
        OnComplete(ErrorResponse(TEXT("At least one node specification is required"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    Body->TryGetArrayField(TEXT("connections"), ConnectionValues);

    TSet<FString> Keys;
    for (const TSharedPtr<FJsonValue>& Value : *NodeValues)
    {
        const TSharedPtr<FJsonObject> Spec = Value.IsValid() ? Value->AsObject() : nullptr;
        FString Key;
        FString Type;
        if (!Spec.IsValid()
            || !Spec->TryGetStringField(TEXT("key"), Key)
            || !Spec->TryGetStringField(TEXT("type"), Type)
            || Key.IsEmpty() || Keys.Contains(Key))
        {
            OnComplete(ErrorResponse(TEXT("Every node needs a unique key and type"),
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
        Keys.Add(Key);

        if (Type == TEXT("call_function"))
        {
            FString FunctionPath;
            Spec->TryGetStringField(TEXT("function_path"), FunctionPath);
            if (!ResolveFunction(FunctionPath))
            {
                OnComplete(ErrorResponse(TEXT("Function not found: ") + FunctionPath,
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
        }
        else if (Type == TEXT("variable_get") || Type == TEXT("variable_set"))
        {
            FString VariableName;
            Spec->TryGetStringField(TEXT("variable_name"), VariableName);
            if (FBlueprintEditorUtils::FindNewVariableIndex(
                Blueprint, FName(*VariableName)) == INDEX_NONE)
            {
                OnComplete(ErrorResponse(TEXT("Blueprint variable not found: ") + VariableName,
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
        }
        else if (Type != TEXT("custom_event") && Type != TEXT("branch")
            && Type != TEXT("sequence") && Type != TEXT("input_key")
            && Type != TEXT("enhanced_input_action"))
        {
            OnComplete(ErrorResponse(TEXT("Unsupported node type: ") + Type,
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
    }

    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetStringField(TEXT("operation"), TEXT("apply_graph_patch"));
        Preview->SetStringField(TEXT("blueprint_path"), Blueprint->GetPathName());
        Preview->SetStringField(TEXT("graph_name"), Graph->GetName());
        Preview->SetNumberField(TEXT("nodes_to_create"), NodeValues->Num());
        Preview->SetNumberField(TEXT("connections_to_create"),
            ConnectionValues ? ConnectionValues->Num() : 0);
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled)
    {
        OnComplete(ErrorResponse(TEXT("Edit mode is locked"),
            EHttpServerResponseCodes::Forbidden));
        return true;
    }

    const FScopedTransaction Transaction(NSLOCTEXT(
        "CodexBlueprintBridge", "ApplyGraphPatch", "Codex: Apply Blueprint Graph Patch"));
    Blueprint->Modify();
    Graph->Modify();
    TMap<FString, UEdGraphNode*> NewNodes;
    TArray<TPair<UEdGraphPin*, UEdGraphPin*>> NewLinks;

    auto Rollback = [&]()
    {
        for (const TPair<UEdGraphPin*, UEdGraphPin*>& Link : NewLinks)
        {
            if (Link.Key && Link.Value)
            {
                Link.Key->BreakLinkTo(Link.Value);
            }
        }
        for (const TPair<FString, UEdGraphNode*>& Pair : NewNodes)
        {
            if (Pair.Value)
            {
                Graph->RemoveNode(Pair.Value);
            }
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
    };

    for (const TSharedPtr<FJsonValue>& Value : *NodeValues)
    {
        const TSharedPtr<FJsonObject> Spec = Value->AsObject();
        const FString Key = Spec->GetStringField(TEXT("key"));
        const FString Type = Spec->GetStringField(TEXT("type"));
        int32 X = 0;
        int32 Y = 0;
        double Number = 0;
        if (Spec->TryGetNumberField(TEXT("x"), Number)) X = static_cast<int32>(Number);
        if (Spec->TryGetNumberField(TEXT("y"), Number)) Y = static_cast<int32>(Number);

        UK2Node* Node = nullptr;
        if (Type == TEXT("custom_event"))
        {
            UK2Node_CustomEvent* Event = NewObject<UK2Node_CustomEvent>(Graph);
            FString EventName;
            Spec->TryGetStringField(TEXT("event_name"), EventName);
            Event->CustomFunctionName = FName(*EventName);
            Node = Event;
        }
        else if (Type == TEXT("call_function"))
        {
            UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph);
            Call->SetFromFunction(ResolveFunction(Spec->GetStringField(TEXT("function_path"))));
            Node = Call;
        }
        else if (Type == TEXT("branch"))
        {
            Node = NewObject<UK2Node_IfThenElse>(Graph);
        }
        else if (Type == TEXT("sequence"))
        {
            Node = NewObject<UK2Node_ExecutionSequence>(Graph);
        }
        else if (Type == TEXT("variable_get"))
        {
            UK2Node_VariableGet* Get = NewObject<UK2Node_VariableGet>(Graph);
            Get->VariableReference.SetSelfMember(FName(*Spec->GetStringField(TEXT("variable_name"))));
            Node = Get;
        }
        else if (Type == TEXT("variable_set"))
        {
            UK2Node_VariableSet* Set = NewObject<UK2Node_VariableSet>(Graph);
            Set->VariableReference.SetSelfMember(FName(*Spec->GetStringField(TEXT("variable_name"))));
            Node = Set;
        }
        else if (Type == TEXT("input_key"))
        {
            FString KeyName;
            Spec->TryGetStringField(TEXT("key_name"), KeyName);
            const FKey InputKeyValue{FName(*KeyName)};
            if (!InputKeyValue.IsValid())
            {
                Rollback();
                OnComplete(ErrorResponse(TEXT("Unknown input key: ") + KeyName,
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
            UK2Node_InputKey* Input = NewObject<UK2Node_InputKey>(Graph);
            Input->InputKey = InputKeyValue;
            bool bConsumeInput = true;
            bool bExecuteWhenPaused = false;
            Spec->TryGetBoolField(TEXT("consume_input"), bConsumeInput);
            Spec->TryGetBoolField(TEXT("execute_when_paused"), bExecuteWhenPaused);
            Input->bConsumeInput = bConsumeInput;
            Input->bExecuteWhenPaused = bExecuteWhenPaused;
            Node = Input;
        }
        else if (Type == TEXT("enhanced_input_action"))
        {
            FString ActionPath;
            Spec->TryGetStringField(TEXT("action_path"), ActionPath);
            UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionPath);
            if (!Action)
            {
                Rollback();
                OnComplete(ErrorResponse(TEXT("Input Action not found: ") + ActionPath,
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
            UK2Node_EnhancedInputAction* Input = NewObject<UK2Node_EnhancedInputAction>(Graph);
            Input->InputAction = Action;
            Node = Input;
        }

        if (!Node)
        {
            Rollback();
            OnComplete(ErrorResponse(TEXT("Could not construct node: ") + Key,
                EHttpServerResponseCodes::ServerError));
            return true;
        }
        Graph->AddNode(Node, true, false);
        Node->CreateNewGuid();
        Node->NodePosX = X;
        Node->NodePosY = Y;
        Node->PostPlacedNewNode();
        Node->AllocateDefaultPins();
        NewNodes.Add(Key, Node);

        const TSharedPtr<FJsonObject>* Defaults = nullptr;
        if (Spec->TryGetObjectField(TEXT("pin_defaults"), Defaults) && Defaults && Defaults->IsValid())
        {
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Default : (*Defaults)->Values)
            {
                UEdGraphPin* Pin = Node->FindPin(FName(*Default.Key));
                if (!Pin || Pin->Direction != EGPD_Input)
                {
                    Rollback();
                    OnComplete(ErrorResponse(TEXT("Invalid pin default on ") + Key + TEXT(":") + Default.Key,
                        EHttpServerResponseCodes::BadRequest));
                    return true;
                }
                GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(
                    *Pin, Default.Value->AsString());
            }
        }
    }

    auto ResolveNode = [&](const FString& Reference) -> UEdGraphNode*
    {
        if (UEdGraphNode** Found = NewNodes.Find(Reference))
        {
            return *Found;
        }
        FGuid Guid;
        if (FGuid::Parse(Reference, Guid))
        {
            for (UEdGraphNode* Existing : Graph->Nodes)
            {
                if (Existing && Existing->NodeGuid == Guid)
                {
                    return Existing;
                }
            }
        }
        return nullptr;
    };

    if (ConnectionValues)
    {
        const UEdGraphSchema* Schema = Graph->GetSchema();
        for (const TSharedPtr<FJsonValue>& Value : *ConnectionValues)
        {
            const TSharedPtr<FJsonObject> Connection = Value.IsValid() ? Value->AsObject() : nullptr;
            if (!Connection.IsValid())
            {
                Rollback();
                OnComplete(ErrorResponse(TEXT("Invalid connection specification"),
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
            const FString FromNodeRef = Connection->GetStringField(TEXT("from_node"));
            const FString ToNodeRef = Connection->GetStringField(TEXT("to_node"));
            UEdGraphNode* FromNode = ResolveNode(FromNodeRef);
            UEdGraphNode* ToNode = ResolveNode(ToNodeRef);
            UEdGraphPin* FromPin = FromNode
                ? FromNode->FindPin(FName(*Connection->GetStringField(TEXT("from_pin")))) : nullptr;
            UEdGraphPin* ToPin = ToNode
                ? ToNode->FindPin(FName(*Connection->GetStringField(TEXT("to_pin")))) : nullptr;
            if (!FromPin || !ToPin || FromPin->Direction != EGPD_Output
                || ToPin->Direction != EGPD_Input || !Schema->TryCreateConnection(FromPin, ToPin))
            {
                Rollback();
                OnComplete(ErrorResponse(TEXT("Unreal rejected a pin connection"),
                    EHttpServerResponseCodes::BadRequest));
                return true;
            }
            NewLinks.Add(TPair<UEdGraphPin*, UEdGraphPin*>(FromPin, ToPin));
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        Rollback();
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint = Blueprint;

    TArray<TSharedPtr<FJsonValue>> Created;
    for (const TPair<FString, UEdGraphNode*>& Pair : NewNodes)
    {
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("key"), Pair.Key);
        Item->SetStringField(TEXT("node_id"), Pair.Value->NodeGuid.ToString());
        Item->SetStringField(TEXT("title"),
            Pair.Value->GetNodeTitle(ENodeTitleType::ListView).ToString());
        Created.Add(MakeShared<FJsonValueObject>(Item));
    }
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetStringField(TEXT("operation"), TEXT("apply_graph_patch"));
    Result->SetStringField(TEXT("compile_status"), TEXT("success"));
    Result->SetArrayField(TEXT("created_nodes"), Created);
    Result->SetNumberField(TEXT("connections_created"), NewLinks.Num());
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleClassDefaults(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    const FString Path = QueryValue(Request, TEXT("path"));
    UBlueprint* Blueprint = Path.StartsWith(TEXT("/Game/"))
        ? LoadObject<UBlueprint>(nullptr, *Path) : nullptr;
    UObject* Defaults = Blueprint && Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    if (!Defaults)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint class defaults are unavailable"),
            EHttpServerResponseCodes::NotFound));
        return true;
    }

    TArray<TSharedPtr<FJsonValue>> Properties;
    TArray<TSharedPtr<FJsonValue>> Components;
    for (TFieldIterator<FProperty> It(Defaults->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FProperty* Property = *It;
        if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            continue;
        }
        Properties.Add(MakeShared<FJsonValueObject>(ReflectedPropertyToJson(Defaults, Property)));
        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            UObject* Object = ObjectProperty->GetObjectPropertyValue(
                Property->ContainerPtrToValuePtr<void>(Defaults));
            if (Object && Object->IsA<UActorComponent>())
            {
                const TSharedRef<FJsonObject> Component = MakeShared<FJsonObject>();
                Component->SetStringField(TEXT("name"), Property->GetName());
                Component->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
                Components.Add(MakeShared<FJsonValueObject>(Component));
            }
        }
    }

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetStringField(TEXT("blueprint_path"), Blueprint->GetPathName());
    Result->SetStringField(TEXT("class"), Blueprint->GeneratedClass->GetPathName());
    Result->SetArrayField(TEXT("properties"), Properties);
    Result->SetArrayField(TEXT("components"), Components);
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleSetDefaultProperty(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString Error;
    if (!ParseJsonBody(Request, Body, Error))
    {
        OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest));
        return true;
    }
    const FString BlueprintPath = Body->GetStringField(TEXT("blueprint_path"));
    const FString PropertyPath = Body->GetStringField(TEXT("property_path"));
    const FString NewValue = Body->GetStringField(TEXT("value"));
    bool bDryRun = true;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);
    UBlueprint* Blueprint = BlueprintPath.StartsWith(TEXT("/Game/"))
        ? LoadObject<UBlueprint>(nullptr, *BlueprintPath) : nullptr;
    UObject* Defaults = Blueprint && Blueprint->GeneratedClass
        ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
    UObject* Owner = nullptr;
    FProperty* Property = nullptr;
    void* ValueAddress = nullptr;
    if (!ResolvePropertyPath(Defaults, PropertyPath, Owner, Property, ValueAddress, Error))
    {
        OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest));
        return true;
    }
    FString OldValue;
    Property->ExportTextItem_Direct(OldValue, ValueAddress, nullptr, Owner, PPF_None);
    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetStringField(TEXT("property_path"), PropertyPath);
        Preview->SetStringField(TEXT("property_type"), Property->GetCPPType());
        Preview->SetStringField(TEXT("old_value"), OldValue);
        Preview->SetStringField(TEXT("new_value"), NewValue);
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled)
    {
        OnComplete(ErrorResponse(TEXT("Edit mode is locked"), EHttpServerResponseCodes::Forbidden));
        return true;
    }

    const FScopedTransaction Transaction(NSLOCTEXT(
        "CodexBlueprintBridge", "SetDefaultProperty", "Codex: Set Class Default Property"));
    Defaults->Modify();
    if (Owner && Owner != Defaults) Owner->Modify();
    const TCHAR* ImportResult = Property->ImportText_Direct(
        *NewValue, ValueAddress, Owner, PPF_None);
    if (!ImportResult)
    {
        OnComplete(ErrorResponse(
            FString::Printf(TEXT("Value '%s' is invalid for %s"), *NewValue, *Property->GetCPPType()),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    FPropertyChangedEvent ChangedEvent(Property);
    Owner->PostEditChangeProperty(ChangedEvent);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint = Blueprint;
    FString SavedValue;
    Property->ExportTextItem_Direct(SavedValue, ValueAddress, nullptr, Owner, PPF_None);
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetStringField(TEXT("property_path"), PropertyPath);
    Result->SetStringField(TEXT("old_value"), OldValue);
    Result->SetStringField(TEXT("saved_value"), SavedValue);
    Result->SetStringField(TEXT("compile_status"), TEXT("success"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleEditGraph(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString Error;
    if (!ParseJsonBody(Request, Body, Error))
    {
        OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest));
        return true;
    }
    const FString BlueprintPath = Body->GetStringField(TEXT("blueprint_path"));
    const FString GraphName = Body->GetStringField(TEXT("graph_name"));
    bool bDryRun = true;
    bool bConfirmDestructive = false;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);
    Body->TryGetBoolField(TEXT("confirm_destructive"), bConfirmDestructive);
    UBlueprint* Blueprint = BlueprintPath.StartsWith(TEXT("/Game/"))
        ? LoadObject<UBlueprint>(nullptr, *BlueprintPath) : nullptr;
    UEdGraph* Graph = Blueprint ? FindBlueprintGraph(Blueprint, GraphName) : nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Operations = nullptr;
    if (!Graph || !Body->TryGetArrayField(TEXT("operations"), Operations) || !Operations)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint, graph, or operations are invalid"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    TMap<FGuid, UEdGraphNode*> NodesById;
    for (UEdGraphNode* Node : Graph->Nodes) if (Node) NodesById.Add(Node->NodeGuid, Node);
    bool bHasDelete = false;
    TArray<TSharedPtr<FJsonValue>> Affected;
    for (const TSharedPtr<FJsonValue>& Value : *Operations)
    {
        const TSharedPtr<FJsonObject> Operation = Value->AsObject();
        FString Type;
        FString NodeId;
        Operation->TryGetStringField(TEXT("type"), Type);
        Operation->TryGetStringField(TEXT("node_id"), NodeId);
        FGuid Guid;
        if (!FGuid::Parse(NodeId, Guid) || !NodesById.Contains(Guid))
        {
            OnComplete(ErrorResponse(TEXT("Unknown node_id: ") + NodeId,
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
        bHasDelete |= Type == TEXT("delete_node");
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("type"), Type);
        Item->SetStringField(TEXT("node_id"), NodeId);
        Item->SetStringField(TEXT("title"), NodesById[Guid]->GetNodeTitle(ENodeTitleType::ListView).ToString());
        Affected.Add(MakeShared<FJsonValueObject>(Item));
    }
    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetBoolField(TEXT("destructive_confirmation_required"), bHasDelete);
        Preview->SetArrayField(TEXT("affected"), Affected);
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled || (bHasDelete && !bConfirmDestructive))
    {
        OnComplete(ErrorResponse(TEXT("Edit mode or destructive confirmation is missing"),
            EHttpServerResponseCodes::Forbidden));
        return true;
    }

    const FScopedTransaction Transaction(NSLOCTEXT(
        "CodexBlueprintBridge", "EditGraph", "Codex: Edit Blueprint Graph"));
    Blueprint->Modify();
    Graph->Modify();
    for (const TSharedPtr<FJsonValue>& Value : *Operations)
    {
        const TSharedPtr<FJsonObject> Operation = Value->AsObject();
        const FString Type = Operation->GetStringField(TEXT("type"));
        FGuid Guid;
        FGuid::Parse(Operation->GetStringField(TEXT("node_id")), Guid);
        UEdGraphNode* Node = NodesById[Guid];
        Node->Modify();
        if (Type == TEXT("move_node"))
        {
            double X = Node->NodePosX;
            double Y = Node->NodePosY;
            Operation->TryGetNumberField(TEXT("x"), X);
            Operation->TryGetNumberField(TEXT("y"), Y);
            Node->NodePosX = static_cast<int32>(X);
            Node->NodePosY = static_cast<int32>(Y);
        }
        else if (Type == TEXT("break_pin_links"))
        {
            UEdGraphPin* Pin = Node->FindPin(FName(*Operation->GetStringField(TEXT("pin"))));
            if (Pin) Pin->BreakAllPinLinks();
        }
        else if (Type == TEXT("delete_node"))
        {
            Node->BreakAllNodeLinks();
            FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
        }
        else
        {
            OnComplete(ErrorResponse(TEXT("Unsupported graph edit operation: ") + Type,
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint = Blueprint;
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetArrayField(TEXT("affected"), Affected);
    Result->SetStringField(TEXT("compile_status"), TEXT("success"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleUndo(
    const FHttpServerRequest&,
    const FHttpResultCallback& OnComplete)
{
    if (!bEditModeEnabled || !GEditor || !LastEditedBlueprint.IsValid())
    {
        OnComplete(ErrorResponse(TEXT("No bridge transaction is available to undo"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }
    if (!GEditor->UndoTransaction())
    {
        OnComplete(ErrorResponse(TEXT("Unreal could not undo the last transaction"),
            EHttpServerResponseCodes::ServerError));
        return true;
    }
    UBlueprint* Blueprint = LastEditedBlueprint.Get();
    FString SaveError;
    if (!SaveBlueprint(Blueprint, SaveError))
    {
        OnComplete(ErrorResponse(SaveError, EHttpServerResponseCodes::ServerError));
        return true;
    }
    LastEditedBlueprint.Reset();
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetStringField(TEXT("status"), TEXT("last bridge transaction undone and saved"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleEnhancedInput(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString Error;
    if (!ParseJsonBody(Request, Body, Error))
    {
        OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest));
        return true;
    }
    const FString Operation = Body->GetStringField(TEXT("operation"));
    bool bDryRun = true;
    Body->TryGetBoolField(TEXT("dry_run"), bDryRun);
    if (bDryRun)
    {
        const TSharedRef<FJsonObject> Preview = MakeShared<FJsonObject>();
        Preview->SetBoolField(TEXT("ok"), true);
        Preview->SetBoolField(TEXT("dry_run"), true);
        Preview->SetStringField(TEXT("operation"), Operation);
        Preview->SetObjectField(TEXT("request"), Body.ToSharedRef());
        OnComplete(JsonResponse(Preview));
        return true;
    }
    if (!bEditModeEnabled)
    {
        OnComplete(ErrorResponse(TEXT("Edit mode is locked"), EHttpServerResponseCodes::Forbidden));
        return true;
    }

    UObject* SavedAsset = nullptr;
    if (Operation == TEXT("create_input_action"))
    {
        const FString PackageName = Body->GetStringField(TEXT("package_name"));
        if (!PackageName.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(PackageName))
        {
            OnComplete(ErrorResponse(TEXT("Invalid /Game package_name"), EHttpServerResponseCodes::BadRequest));
            return true;
        }
        UPackage* Package = CreatePackage(*PackageName);
        UInputAction* Action = NewObject<UInputAction>(Package,
            FName(*FPackageName::GetLongPackageAssetName(PackageName)), RF_Public | RF_Standalone);
        FString ValueType = TEXT("bool");
        Body->TryGetStringField(TEXT("value_type"), ValueType);
        if (ValueType == TEXT("axis1d")) Action->ValueType = EInputActionValueType::Axis1D;
        else if (ValueType == TEXT("axis2d")) Action->ValueType = EInputActionValueType::Axis2D;
        else if (ValueType == TEXT("axis3d")) Action->ValueType = EInputActionValueType::Axis3D;
        else Action->ValueType = EInputActionValueType::Boolean;
        FAssetRegistryModule::AssetCreated(Action);
        SavedAsset = Action;
    }
    else if (Operation == TEXT("create_mapping_context"))
    {
        const FString PackageName = Body->GetStringField(TEXT("package_name"));
        if (!PackageName.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(PackageName))
        {
            OnComplete(ErrorResponse(TEXT("Invalid /Game package_name"), EHttpServerResponseCodes::BadRequest));
            return true;
        }
        UPackage* Package = CreatePackage(*PackageName);
        UInputMappingContext* Context = NewObject<UInputMappingContext>(Package,
            FName(*FPackageName::GetLongPackageAssetName(PackageName)), RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(Context);
        SavedAsset = Context;
    }
    else if (Operation == TEXT("add_mapping"))
    {
        UInputMappingContext* Context = LoadObject<UInputMappingContext>(
            nullptr, *Body->GetStringField(TEXT("context_path")));
        UInputAction* Action = LoadObject<UInputAction>(
            nullptr, *Body->GetStringField(TEXT("action_path")));
        const FKey Key(FName(*Body->GetStringField(TEXT("key_name"))));
        if (!Context || !Action || !Key.IsValid())
        {
            OnComplete(ErrorResponse(TEXT("Context, action, or key is invalid"),
                EHttpServerResponseCodes::BadRequest));
            return true;
        }
        const FScopedTransaction Transaction(NSLOCTEXT(
            "CodexBlueprintBridge", "AddInputMapping", "Codex: Add Enhanced Input Mapping"));
        Context->Modify();
        Context->MapKey(Action, Key);
        SavedAsset = Context;
    }
    else
    {
        OnComplete(ErrorResponse(TEXT("Unsupported Enhanced Input operation"),
            EHttpServerResponseCodes::BadRequest));
        return true;
    }

    UPackage* Package = SavedAsset->GetOutermost();
    Package->MarkPackageDirty();
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    if (!UPackage::SavePackage(Package, SavedAsset, *Filename, SaveArgs))
    {
        OnComplete(ErrorResponse(TEXT("Enhanced Input asset could not be saved"),
            EHttpServerResponseCodes::ServerError));
        return true;
    }
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetBoolField(TEXT("dry_run"), false);
    Result->SetStringField(TEXT("operation"), Operation);
    Result->SetStringField(TEXT("asset_path"), SavedAsset->GetPathName());
    Result->SetStringField(TEXT("save_status"), TEXT("success"));
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleSearchActions(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    const FString Search = QueryValue(Request, TEXT("search"));
    int32 Limit = FMath::Clamp(FCString::Atoi(*QueryValue(Request, TEXT("limit"))), 1, 500);
    if (Limit <= 1) Limit = 50;
    TArray<TSharedPtr<FJsonValue>> Actions;
    for (TObjectIterator<UFunction> It; It && Actions.Num() < Limit; ++It)
    {
        UFunction* Function = *It;
        if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
        {
            continue;
        }
        const FString DisplayName = Function->GetDisplayNameText().ToString();
        const FString Owner = Function->GetOuterUClass()
            ? Function->GetOuterUClass()->GetPathName() : FString();
        if (!Search.IsEmpty()
            && !DisplayName.Contains(Search, ESearchCase::IgnoreCase)
            && !Function->GetName().Contains(Search, ESearchCase::IgnoreCase)
            && !Owner.Contains(Search, ESearchCase::IgnoreCase))
        {
            continue;
        }
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), DisplayName);
        Item->SetStringField(TEXT("function_path"), Owner + TEXT(":") + Function->GetName());
        Item->SetStringField(TEXT("owner"), Owner);
        Item->SetStringField(TEXT("category"), Function->GetMetaData(TEXT("Category")));
        Item->SetBoolField(TEXT("pure"), Function->HasAnyFunctionFlags(FUNC_BlueprintPure));
        Actions.Add(MakeShared<FJsonValueObject>(Item));
    }
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetArrayField(TEXT("actions"), Actions);
    Result->SetNumberField(TEXT("returned"), Actions.Num());
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleDescribeAction(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    const FString Path = QueryValue(Request, TEXT("function_path"));
    UFunction* Function = ResolveFunction(Path);
    if (!Function)
    {
        OnComplete(ErrorResponse(TEXT("Function action not found"), EHttpServerResponseCodes::NotFound));
        return true;
    }
    TArray<TSharedPtr<FJsonValue>> Pins;
    for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        FProperty* Property = *It;
        const TSharedRef<FJsonObject> Pin = MakeShared<FJsonObject>();
        Pin->SetStringField(TEXT("name"), Property->GetName());
        Pin->SetStringField(TEXT("type"), Property->GetCPPType());
        Pin->SetStringField(TEXT("direction"),
            Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm) ? TEXT("output") : TEXT("input"));
        Pin->SetBoolField(TEXT("reference"), Property->HasAnyPropertyFlags(CPF_ReferenceParm));
        Pins.Add(MakeShared<FJsonValueObject>(Pin));
    }
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), true);
    Result->SetStringField(TEXT("function_path"), Path);
    Result->SetStringField(TEXT("display_name"), Function->GetDisplayNameText().ToString());
    Result->SetStringField(TEXT("tooltip"), Function->GetToolTipText().ToString());
    Result->SetBoolField(TEXT("pure"), Function->HasAnyFunctionFlags(FUNC_BlueprintPure));
    Result->SetArrayField(TEXT("pins"), Pins);
    OnComplete(JsonResponse(Result));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleCompileBlueprint(
    const FHttpServerRequest& Request,
    const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;
    FString Error;
    if (!ParseJsonBody(Request, Body, Error))
    {
        OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest));
        return true;
    }
    UBlueprint* Blueprint = LoadObject<UBlueprint>(
        nullptr, *Body->GetStringField(TEXT("blueprint_path")));
    if (!Blueprint)
    {
        OnComplete(ErrorResponse(TEXT("Blueprint not found"), EHttpServerResponseCodes::NotFound));
        return true;
    }
    FCompilerResultsLog Log;
    Log.bSilentMode = true;
    Log.bLogDetailedResults = true;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Log);
    TArray<TSharedPtr<FJsonValue>> Messages;
    for (const TSharedRef<FTokenizedMessage>& Message : Log.Messages)
    {
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("severity"),
            FTokenizedMessage::GetSeverityText(Message->GetSeverity()).ToString());
        Item->SetStringField(TEXT("message"), Message->ToText().ToString());
        Messages.Add(MakeShared<FJsonValueObject>(Item));
    }
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("ok"), Log.NumErrors == 0);
    Result->SetNumberField(TEXT("errors"), Log.NumErrors);
    Result->SetNumberField(TEXT("warnings"), Log.NumWarnings);
    Result->SetArrayField(TEXT("messages"), Messages);
    OnComplete(JsonResponse(Result,
        Log.NumErrors == 0 ? EHttpServerResponseCodes::Ok : EHttpServerResponseCodes::Conflict));
    return true;
}

bool FCodexBlueprintBridgeModule::HandleDuplicateAsset(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body; FString Error;
    if (!ParseJsonBody(Request, Body, Error)) { OnComplete(ErrorResponse(Error, EHttpServerResponseCodes::BadRequest)); return true; }
    FString SourcePath, PackageName; Body->TryGetStringField(TEXT("source_path"), SourcePath); Body->TryGetStringField(TEXT("package_name"), PackageName);
    bool bDryRun = true; Body->TryGetBoolField(TEXT("dry_run"), bDryRun);
    UObject* Source = LoadObject<UObject>(nullptr, *SourcePath);
    if (!Source || !PackageName.StartsWith(TEXT("/Game/"))) { OnComplete(ErrorResponse(TEXT("Valid source_path and /Game package_name are required"), EHttpServerResponseCodes::BadRequest)); return true; }
    const FString NewName = FPackageName::GetLongPackageAssetName(PackageName);
    const FString DestPath = FPackageName::GetLongPackagePath(PackageName);
    if (NewName.IsEmpty() || FindObject<UObject>(nullptr, *(PackageName + TEXT(".") + NewName))) { OnComplete(ErrorResponse(TEXT("Destination name is invalid or already exists"), EHttpServerResponseCodes::Conflict)); return true; }
    if (bDryRun) { const auto R=MakeShared<FJsonObject>(); R->SetBoolField(TEXT("ok"),true); R->SetBoolField(TEXT("dry_run"),true); R->SetStringField(TEXT("source"),SourcePath); R->SetStringField(TEXT("destination"),PackageName); OnComplete(JsonResponse(R)); return true; }
    if (!bEditModeEnabled) { OnComplete(ErrorResponse(TEXT("Edit mode is locked"), EHttpServerResponseCodes::Forbidden)); return true; }
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","DuplicateAsset","Codex: Duplicate asset"));
    UObject* Copy = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().DuplicateAsset(NewName, DestPath, Source);
    if (!Copy) { OnComplete(ErrorResponse(TEXT("Unreal could not duplicate the asset"), EHttpServerResponseCodes::ServerError)); return true; }
    Copy->GetOutermost()->MarkPackageDirty();
    if (UBlueprint* BP=Cast<UBlueprint>(Copy)) { LastEditedBlueprint=BP; SaveBlueprint(BP,Error); }
    const auto R=MakeShared<FJsonObject>(); R->SetBoolField(TEXT("ok"),true); R->SetStringField(TEXT("object_path"),Copy->GetPathName()); OnComplete(JsonResponse(R)); return true;
}

bool FCodexBlueprintBridgeModule::HandleBlueprintFunction(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body; FString Error;
    if (!ParseJsonBody(Request,Body,Error)) { OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest)); return true; }
    FString BPPath, Name, Operation=TEXT("create"); Body->TryGetStringField(TEXT("blueprint_path"),BPPath); Body->TryGetStringField(TEXT("function_name"),Name); Body->TryGetStringField(TEXT("operation"),Operation);
    bool bDryRun=true; Body->TryGetBoolField(TEXT("dry_run"),bDryRun); UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*BPPath);
    if (!BP || Name.IsEmpty()) { OnComplete(ErrorResponse(TEXT("Valid blueprint_path and function_name are required"),EHttpServerResponseCodes::BadRequest)); return true; }
    if (FindBlueprintGraph(BP,Name)) { OnComplete(ErrorResponse(TEXT("A graph with that name already exists"),EHttpServerResponseCodes::Conflict)); return true; }
    UFunction* Signature=nullptr;
    if (Operation.Equals(TEXT("override"),ESearchCase::IgnoreCase) && BP->ParentClass) Signature=BP->ParentClass->FindFunctionByName(*Name);
    if (Operation.Equals(TEXT("override"),ESearchCase::IgnoreCase) && !Signature) { OnComplete(ErrorResponse(TEXT("Override function was not found in the parent class"),EHttpServerResponseCodes::NotFound)); return true; }
    if (bDryRun) { const auto R=MakeShared<FJsonObject>(); R->SetBoolField(TEXT("ok"),true); R->SetBoolField(TEXT("dry_run"),true); R->SetStringField(TEXT("operation"),Operation); R->SetStringField(TEXT("function_name"),Name); OnComplete(JsonResponse(R)); return true; }
    if (!bEditModeEnabled) { OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden)); return true; }
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","CreateFunction","Codex: Create Blueprint function")); BP->Modify();
    UEdGraph* Graph=FBlueprintEditorUtils::CreateNewGraph(BP,*Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP,Graph,Signature==nullptr,Signature);
    if (!Signature)
    {
        FString Category,Access=TEXT("public");bool bPure=false;Body->TryGetStringField(TEXT("category"),Category);Body->TryGetStringField(TEXT("access"),Access);Body->TryGetBoolField(TEXT("pure"),bPure);if(!Category.IsEmpty())FBlueprintEditorUtils::SetBlueprintFunctionOrMacroCategory(Graph,FText::FromString(Category),true);for(UEdGraphNode* N:Graph->Nodes)if(UK2Node_FunctionEntry* Entry=Cast<UK2Node_FunctionEntry>(N)){int32 Flags=FUNC_BlueprintCallable|(bPure?FUNC_BlueprintPure:0);Flags|=Access==TEXT("private")?FUNC_Private:(Access==TEXT("protected")?FUNC_Protected:FUNC_Public);Entry->SetExtraFlags(Flags);}
        const TArray<TSharedPtr<FJsonValue>>* Params=nullptr;
        if (Body->TryGetArrayField(TEXT("parameters"),Params)) for (const auto& V:*Params)
        {
            const auto P=V->AsObject(); FString PName,PType,Direction=TEXT("input"),Default; P->TryGetStringField(TEXT("name"),PName); P->TryGetStringField(TEXT("type"),PType); P->TryGetStringField(TEXT("direction"),Direction);P->TryGetStringField(TEXT("default_value"),Default);
            FEdGraphPinType PT; if (!PName.IsEmpty() && PinTypeFromName(PType,PT))
            {
                if (Direction.Equals(TEXT("output"),ESearchCase::IgnoreCase)) { for (UEdGraphNode* N:Graph->Nodes) if (UK2Node_FunctionResult* X=Cast<UK2Node_FunctionResult>(N)) X->CreateUserDefinedPin(*PName,PT,EGPD_Input); }
                else { for (UEdGraphNode* N:Graph->Nodes) if (UK2Node_FunctionEntry* X=Cast<UK2Node_FunctionEntry>(N)) {UEdGraphPin* NewPin=X->CreateUserDefinedPin(*PName,PT,EGPD_Output);if(NewPin&&!Default.IsEmpty())NewPin->DefaultValue=Default;} }
            }
        }
    }
    if (!SaveBlueprint(BP,Error)) { Tx.Cancel(); OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::Conflict)); return true; }
    LastEditedBlueprint=BP; const auto R=MakeShared<FJsonObject>(); R->SetBoolField(TEXT("ok"),true); R->SetStringField(TEXT("graph_name"),Graph->GetName()); OnComplete(JsonResponse(R)); return true;
}

bool FCodexBlueprintBridgeModule::HandleBlueprintDispatcher(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body; FString Error; if(!ParseJsonBody(Request,Body,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}
    FString BPPath,Name; Body->TryGetStringField(TEXT("blueprint_path"),BPPath); Body->TryGetStringField(TEXT("dispatcher_name"),Name); bool bDryRun=true;Body->TryGetBoolField(TEXT("dry_run"),bDryRun); UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*BPPath);
    if(!BP||Name.IsEmpty()){OnComplete(ErrorResponse(TEXT("Valid blueprint_path and dispatcher_name are required"),EHttpServerResponseCodes::BadRequest));return true;}
    if(bDryRun){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("dispatcher_name"),Name);OnComplete(JsonResponse(R));return true;}
    if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","Dispatcher","Codex: Add event dispatcher")); BP->Modify(); FEdGraphPinType T;T.PinCategory=UEdGraphSchema_K2::PC_MCDelegate;
    if(!FBlueprintEditorUtils::AddMemberVariable(BP,*Name,T)){OnComplete(ErrorResponse(TEXT("Could not add dispatcher variable"),EHttpServerResponseCodes::Conflict));return true;}
    UEdGraph* G=FBlueprintEditorUtils::CreateNewGraph(BP,*Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass()); const UEdGraphSchema_K2* S=GetDefault<UEdGraphSchema_K2>(); G->bEditable=false; S->CreateDefaultNodesForGraph(*G);S->CreateFunctionGraphTerminators(*G,(UClass*)nullptr);S->AddExtraFunctionFlags(G,FUNC_BlueprintCallable|FUNC_BlueprintEvent|FUNC_Public);S->MarkFunctionEntryAsEditable(G,true);BP->DelegateSignatureGraphs.Add(G);FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    if(!SaveBlueprint(BP,Error)){Tx.Cancel();OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::Conflict));return true;} LastEditedBlueprint=BP;const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("dispatcher_name"),Name);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleBlueprintInterface(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;FString Error;if(!ParseJsonBody(Request,Body,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;} FString BPPath,InterfacePath;Body->TryGetStringField(TEXT("blueprint_path"),BPPath);Body->TryGetStringField(TEXT("interface_path"),InterfacePath);bool bDryRun=true;Body->TryGetBoolField(TEXT("dry_run"),bDryRun);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*BPPath);UClass* Interface=LoadObject<UClass>(nullptr,*InterfacePath);
    if(!BP||!Interface||!Interface->HasAnyClassFlags(CLASS_Interface)){OnComplete(ErrorResponse(TEXT("Valid Blueprint and interface class paths are required"),EHttpServerResponseCodes::BadRequest));return true;} if(bDryRun){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("interface"),InterfacePath);OnComplete(JsonResponse(R));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","Interface","Codex: Implement interface"));BP->Modify();if(!FBlueprintEditorUtils::ImplementNewInterface(BP,Interface->GetClassPathName())){OnComplete(ErrorResponse(TEXT("Interface is already implemented or is incompatible"),EHttpServerResponseCodes::Conflict));return true;}if(!SaveBlueprint(BP,Error)){Tx.Cancel();OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("interface"),InterfacePath);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleBlueprintComponent(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> Body;FString Error;if(!ParseJsonBody(Request,Body,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}FString BPPath,ClassPath,Name;Body->TryGetStringField(TEXT("blueprint_path"),BPPath);Body->TryGetStringField(TEXT("component_class"),ClassPath);Body->TryGetStringField(TEXT("component_name"),Name);bool bDryRun=true;Body->TryGetBoolField(TEXT("dry_run"),bDryRun);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*BPPath);UClass* C=LoadObject<UClass>(nullptr,*ClassPath);
    if(!BP||!BP->SimpleConstructionScript||!C||!C->IsChildOf(UActorComponent::StaticClass())||Name.IsEmpty()){OnComplete(ErrorResponse(TEXT("Valid actor Blueprint, component class, and name are required"),EHttpServerResponseCodes::BadRequest));return true;}if(bDryRun){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("component_name"),Name);OnComplete(JsonResponse(R));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","Component","Codex: Add component"));BP->Modify();USCS_Node* Node=BP->SimpleConstructionScript->CreateNode(C,*Name);if(!Node){OnComplete(ErrorResponse(TEXT("Could not create component"),EHttpServerResponseCodes::Conflict));return true;}BP->SimpleConstructionScript->AddNode(Node);FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);if(!SaveBlueprint(BP,Error)){Tx.Cancel();OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("component_name"),Node->GetVariableName().ToString());OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandlePIE(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    FString Operation=TEXT("status"); if(Request.Verb==EHttpServerRequestVerbs::VERB_POST){TSharedPtr<FJsonObject> Body;FString Error;if(!ParseJsonBody(Request,Body,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}Body->TryGetStringField(TEXT("operation"),Operation);}
    if(!GEditor){OnComplete(ErrorResponse(TEXT("Editor is unavailable"),EHttpServerResponseCodes::ServerError));return true;}if(Operation==TEXT("start")&&!GEditor->PlayWorld){GEditor->RequestPlaySession(FRequestPlaySessionParams());}else if(Operation==TEXT("stop")&&GEditor->PlayWorld){GEditor->RequestEndPlayMap();}else if(Operation!=TEXT("status")&&Operation!=TEXT("start")&&Operation!=TEXT("stop")){OnComplete(ErrorResponse(TEXT("operation must be status, start, or stop"),EHttpServerResponseCodes::BadRequest));return true;}
    const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("requested_operation"),Operation);R->SetBoolField(TEXT("playing"),GEditor->PlayWorld!=nullptr);R->SetBoolField(TEXT("starting"),GEditor->IsSettingUpPlayWorld());OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleSnapshot(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject> B;FString E;if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}
    FString Path,Op=TEXT("create"),Id;B->TryGetStringField(TEXT("blueprint_path"),Path);B->TryGetStringField(TEXT("operation"),Op);B->TryGetStringField(TEXT("snapshot_id"),Id);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);
    if(!BP){OnComplete(ErrorResponse(TEXT("Blueprint not found"),EHttpServerResponseCodes::NotFound));return true;}
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("CodexBlueprintBridge/Snapshots");IFileManager::Get().MakeDirectory(*Dir,true);const FString Source=FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    if(Op==TEXT("create"))
    {
        if(!FPaths::FileExists(Source)){OnComplete(ErrorResponse(TEXT("Saved Blueprint package was not found on disk"),EHttpServerResponseCodes::NotFound));return true;}Id=BP->GetName()+TEXT("_")+FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S%f"))+TEXT(".uasset");const FString Dest=Dir/Id;
        if(IFileManager::Get().Copy(*Dest,*Source,true,true)!=COPY_OK){OnComplete(ErrorResponse(TEXT("Could not create snapshot"),EHttpServerResponseCodes::ServerError));return true;}
        const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("snapshot_id"),Id);R->SetStringField(TEXT("blueprint_path"),Path);OnComplete(JsonResponse(R));return true;
    }
    if(Op==TEXT("restore"))
    {
        if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}if(Id.IsEmpty()||Id.Contains(TEXT("/"))||Id.Contains(TEXT("\\"))){OnComplete(ErrorResponse(TEXT("Invalid snapshot_id"),EHttpServerResponseCodes::BadRequest));return true;}const FString Snap=Dir/Id;if(!FPaths::FileExists(Snap)){OnComplete(ErrorResponse(TEXT("Snapshot not found"),EHttpServerResponseCodes::NotFound));return true;}
        if(IFileManager::Get().Copy(*Source,*Snap,true,true)!=COPY_OK){OnComplete(ErrorResponse(TEXT("Could not restore snapshot file"),EHttpServerResponseCodes::ServerError));return true;}FText ReloadError;TArray<UPackage*> Packages{BP->GetOutermost()};if(!UPackageTools::ReloadPackages(Packages,ReloadError,false)){OnComplete(ErrorResponse(TEXT("Snapshot restored on disk but reload failed: ")+ReloadError.ToString(),EHttpServerResponseCodes::Conflict));return true;}const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("restored"),true);R->SetStringField(TEXT("snapshot_id"),Id);OnComplete(JsonResponse(R));return true;
    }
    OnComplete(ErrorResponse(TEXT("operation must be create or restore"),EHttpServerResponseCodes::BadRequest));return true;
}

bool FCodexBlueprintBridgeModule::HandleInspectRegion(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    const FString Path=QueryValue(Request,TEXT("path")),GraphName=QueryValue(Request,TEXT("graph")),Search=QueryValue(Request,TEXT("search"));UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);UEdGraph* G=BP?FindBlueprintGraph(BP,GraphName):nullptr;if(!G){OnComplete(ErrorResponse(TEXT("Blueprint or graph not found"),EHttpServerResponseCodes::NotFound));return true;}int32 Limit=FMath::Clamp(FCString::Atoi(*QueryValue(Request,TEXT("limit"))),1,500);if(Limit<=1)Limit=100;
    FString Needle=Search;Needle.ReplaceInline(TEXT(" "),TEXT(""));Needle.ReplaceInline(TEXT("_"),TEXT(""));Needle.ToLowerInline();TArray<TSharedPtr<FJsonValue>> Nodes;for(const UEdGraphNode* N:G->Nodes){if(!N)continue;const FString Title=N->GetNodeTitle(ENodeTitleType::ListView).ToString();FString Haystack=Title+N->GetClass()->GetName()+N->NodeGuid.ToString();if(const UK2Node_CallFunction* Call=Cast<UK2Node_CallFunction>(N))Haystack+=Call->FunctionReference.GetMemberName().ToString();if(const UK2Node_VariableGet* Get=Cast<UK2Node_VariableGet>(N))Haystack+=Get->VariableReference.GetMemberName().ToString();if(const UK2Node_VariableSet* Set=Cast<UK2Node_VariableSet>(N))Haystack+=Set->VariableReference.GetMemberName().ToString();Haystack.ReplaceInline(TEXT(" "),TEXT(""));Haystack.ReplaceInline(TEXT("_"),TEXT(""));Haystack.ToLowerInline();if(!Needle.IsEmpty()&&!Haystack.Contains(Needle))continue;const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),N->NodeGuid.ToString());J->SetStringField(TEXT("class"),N->GetClass()->GetName());J->SetStringField(TEXT("title"),Title);J->SetNumberField(TEXT("x"),N->NodePosX);J->SetNumberField(TEXT("y"),N->NodePosY);TArray<TSharedPtr<FJsonValue>> Pins;for(const UEdGraphPin* P:N->Pins)if(P)Pins.Add(MakeShared<FJsonValueObject>(PinToJson(P)));J->SetArrayField(TEXT("pins"),Pins);Nodes.Add(MakeShared<FJsonValueObject>(J));if(Nodes.Num()>=Limit)break;}const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("graph"),G->GetName());R->SetNumberField(TEXT("returned"),Nodes.Num());R->SetArrayField(TEXT("nodes"),Nodes);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleComponentDetails(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    FString Path,Name;TSharedPtr<FJsonObject>B;FString E;if(Request.Verb==EHttpServerRequestVerbs::VERB_GET){Path=QueryValue(Request,TEXT("path"));Name=QueryValue(Request,TEXT("component"));}else if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}else{B->TryGetStringField(TEXT("blueprint_path"),Path);B->TryGetStringField(TEXT("component_name"),Name);}UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);USCS_Node* Node=BP&&BP->SimpleConstructionScript?BP->SimpleConstructionScript->FindSCSNode(*Name):nullptr;UActorComponent* C=Node?Cast<UActorComponent>(Node->ComponentTemplate):nullptr;if(!C){OnComplete(ErrorResponse(TEXT("SCS component not found"),EHttpServerResponseCodes::NotFound));return true;}
    if(Request.Verb==EHttpServerRequestVerbs::VERB_POST){bool Dry=true;B->TryGetBoolField(TEXT("dry_run"),Dry);const TArray<TSharedPtr<FJsonValue>>* Tags=nullptr;B->TryGetArrayField(TEXT("tags"),Tags);if(!Dry&&!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}if(!Dry&&Tags){FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","ComponentTags","Codex: Set component tags"));C->Modify();C->ComponentTags.Reset();for(const auto& V:*Tags)C->ComponentTags.Add(*V->AsString());FBlueprintEditorUtils::MarkBlueprintAsModified(BP);if(!SaveBlueprint(BP,E)){Tx.Cancel();OnComplete(ErrorResponse(E,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;}}
    const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("component_name"),Node->GetVariableName().ToString());R->SetStringField(TEXT("component_class"),C->GetClass()->GetPathName());TArray<TSharedPtr<FJsonValue>> Tags;for(const FName& T:C->ComponentTags)Tags.Add(MakeShared<FJsonValueString>(T.ToString()));R->SetArrayField(TEXT("tags"),Tags);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleAnalyzeExtractFunction(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString E;if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}FString Path,GraphName;B->TryGetStringField(TEXT("blueprint_path"),Path);B->TryGetStringField(TEXT("graph_name"),GraphName);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);UEdGraph* G=BP?FindBlueprintGraph(BP,GraphName):nullptr;const TArray<TSharedPtr<FJsonValue>>* Ids=nullptr;B->TryGetArrayField(TEXT("node_ids"),Ids);if(!G||!Ids||Ids->Num()==0){OnComplete(ErrorResponse(TEXT("Valid graph and node_ids are required"),EHttpServerResponseCodes::BadRequest));return true;}TSet<FGuid> Selected;for(const auto& V:*Ids){FGuid X;if(FGuid::Parse(V->AsString(),X))Selected.Add(X);}int32 ExecIn=0,ExecOut=0,DataIn=0,DataOut=0;TArray<TSharedPtr<FJsonValue>> Reasons;for(const UEdGraphNode* N:G->Nodes)if(N&&Selected.Contains(N->NodeGuid)){const FString Label=N->GetClass()->GetName()+TEXT(" ")+N->GetNodeTitle(ENodeTitleType::ListView).ToString();if(Label.Contains(TEXT("Delay"))||Label.Contains(TEXT("Timeline"))||Label.Contains(TEXT("Async")))Reasons.Add(MakeShared<FJsonValueString>(TEXT("Latent or asynchronous node: ")+Label));for(const UEdGraphPin* P:N->Pins)if(P){if(P->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard)Reasons.Add(MakeShared<FJsonValueString>(TEXT("Unresolved wildcard pin: ")+P->PinName.ToString()));for(const UEdGraphPin* L:P->LinkedTo)if(L&&L->GetOwningNode()&&!Selected.Contains(L->GetOwningNode()->NodeGuid)){const bool Exec=P->PinType.PinCategory==UEdGraphSchema_K2::PC_Exec;if(P->Direction==EGPD_Input){if(Exec)ExecIn++;else DataIn++;}else{if(Exec)ExecOut++;else DataOut++;}}}}
    if(ExecIn>1)Reasons.Add(MakeShared<FJsonValueString>(TEXT("More than one external execution input")));if(ExecOut>1)Reasons.Add(MakeShared<FJsonValueString>(TEXT("More than one external execution output")));const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("extractable"),Reasons.Num()==0);R->SetNumberField(TEXT("selected_nodes"),Selected.Num());R->SetNumberField(TEXT("exec_inputs"),ExecIn);R->SetNumberField(TEXT("exec_outputs"),ExecOut);R->SetNumberField(TEXT("data_inputs"),DataIn);R->SetNumberField(TEXT("data_outputs"),DataOut);R->SetArrayField(TEXT("reasons"),Reasons);R->SetStringField(TEXT("next_step"),Reasons.Num()==0?TEXT("Create snapshot, then extract with a future apply operation"):TEXT("Adjust the selection"));OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleWidgetTree(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    FString Path;TSharedPtr<FJsonObject>B;FString Error;
    if(Request.Verb==EHttpServerRequestVerbs::VERB_GET)Path=QueryValue(Request,TEXT("path"));
    else if(!ParseJsonBody(Request,B,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}
    else B->TryGetStringField(TEXT("widget_blueprint_path"),Path);
    UWidgetBlueprint* BP=LoadObject<UWidgetBlueprint>(nullptr,*Path);
    if(!BP||!BP->WidgetTree){OnComplete(ErrorResponse(TEXT("Widget Blueprint not found"),EHttpServerResponseCodes::NotFound));return true;}
    if(Request.Verb==EHttpServerRequestVerbs::VERB_GET)
    {
        TArray<UWidget*> All;BP->WidgetTree->GetAllWidgets(All);TArray<TSharedPtr<FJsonValue>> Items;
        for(UWidget* W:All){if(!W)continue;int32 Index=INDEX_NONE;UPanelWidget* Parent=UWidgetTree::FindWidgetParent(W,Index);const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),W->GetName());J->SetStringField(TEXT("class"),W->GetClass()->GetPathName());J->SetStringField(TEXT("parent"),Parent?Parent->GetName():TEXT(""));J->SetNumberField(TEXT("index"),Index);J->SetBoolField(TEXT("is_variable"),W->bIsVariable);J->SetStringField(TEXT("visibility"),StaticEnum<ESlateVisibility>()->GetNameStringByValue((int64)W->GetVisibility()));if(UCanvasPanelSlot* S=Cast<UCanvasPanelSlot>(W->Slot)){J->SetNumberField(TEXT("x"),S->GetPosition().X);J->SetNumberField(TEXT("y"),S->GetPosition().Y);J->SetNumberField(TEXT("width"),S->GetSize().X);J->SetNumberField(TEXT("height"),S->GetSize().Y);J->SetNumberField(TEXT("z_order"),S->GetZOrder());J->SetBoolField(TEXT("auto_size"),S->GetAutoSize());}Items.Add(MakeShared<FJsonValueObject>(J));}
        const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("root"),BP->WidgetTree->RootWidget?BP->WidgetTree->RootWidget->GetName():TEXT(""));R->SetArrayField(TEXT("widgets"),Items);OnComplete(JsonResponse(R));return true;
    }
    FString Op,Name,ParentName,ClassPath;B->TryGetStringField(TEXT("operation"),Op);B->TryGetStringField(TEXT("widget_name"),Name);B->TryGetStringField(TEXT("parent_name"),ParentName);B->TryGetStringField(TEXT("widget_class"),ClassPath);bool Dry=true,Confirm=false;B->TryGetBoolField(TEXT("dry_run"),Dry);B->TryGetBoolField(TEXT("confirm_destructive"),Confirm);
    UWidget* Existing=Name.IsEmpty()?nullptr:BP->WidgetTree->FindWidget(*Name);UWidget* ParentWidget=ParentName.IsEmpty()?nullptr:BP->WidgetTree->FindWidget(*ParentName);
    if(Op==TEXT("add")&&(Name.IsEmpty()||ClassPath.IsEmpty()||Existing)){OnComplete(ErrorResponse(TEXT("add requires a unique widget_name and widget_class"),EHttpServerResponseCodes::Conflict));return true;}
    if((Op==TEXT("remove")||Op==TEXT("set_variable")||Op==TEXT("set_slot")||Op==TEXT("set_property")||Op==TEXT("set_brush")||Op==TEXT("reparent"))&&!Existing){OnComplete(ErrorResponse(TEXT("Target widget not found"),EHttpServerResponseCodes::NotFound));return true;}
    if(Dry){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("operation"),Op);R->SetStringField(TEXT("widget_name"),Name);R->SetStringField(TEXT("parent_name"),ParentName);OnComplete(JsonResponse(R));return true;}
    if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}if((Op==TEXT("remove")||Op==TEXT("reparent"))&&!Confirm){OnComplete(ErrorResponse(TEXT("Destructive confirmation is required"),EHttpServerResponseCodes::Forbidden));return true;}
    const FString SnapshotDir=FPaths::ProjectSavedDir()/TEXT("CodexBlueprintBridge/Snapshots");IFileManager::Get().MakeDirectory(*SnapshotDir,true);const FString Source=FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());const FString Snapshot=SnapshotDir/(BP->GetName()+TEXT("_umg_")+FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S%f"))+TEXT(".uasset"));if(FPaths::FileExists(Source))IFileManager::Get().Copy(*Snapshot,*Source,true,true);
    FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","EditWidgetTree","Codex: Edit Widget Tree"));BP->Modify();BP->WidgetTree->Modify();
    if(Op==TEXT("add"))
    {
        UClass* WC=LoadObject<UClass>(nullptr,*ClassPath);if(!WC||!WC->IsChildOf(UWidget::StaticClass())){Tx.Cancel();OnComplete(ErrorResponse(TEXT("widget_class is not a UWidget class"),EHttpServerResponseCodes::BadRequest));return true;}UWidget* NewWidget=BP->WidgetTree->ConstructWidget<UWidget>(WC,*Name);if(!NewWidget){Tx.Cancel();OnComplete(ErrorResponse(TEXT("Could not construct widget"),EHttpServerResponseCodes::ServerError));return true;}if(!BP->WidgetTree->RootWidget&&ParentName.IsEmpty())BP->WidgetTree->RootWidget=NewWidget;else{UPanelWidget* Panel=Cast<UPanelWidget>(ParentWidget);if(!Panel){Tx.Cancel();OnComplete(ErrorResponse(TEXT("parent_name must be a panel, or omit it for an empty root"),EHttpServerResponseCodes::BadRequest));return true;}Panel->Modify();Panel->AddChild(NewWidget);}Existing=NewWidget;if(UCanvasPanelSlot*S=Cast<UCanvasPanelSlot>(Existing->Slot)){double X,Y,W,H;if(B->TryGetNumberField(TEXT("x"),X)||B->TryGetNumberField(TEXT("y"),Y)){X=B->HasField(TEXT("x"))?B->GetNumberField(TEXT("x")):S->GetPosition().X;Y=B->HasField(TEXT("y"))?B->GetNumberField(TEXT("y")):S->GetPosition().Y;S->SetPosition(FVector2D(X,Y));}if(B->TryGetNumberField(TEXT("width"),W)||B->TryGetNumberField(TEXT("height"),H)){W=B->HasField(TEXT("width"))?B->GetNumberField(TEXT("width")):S->GetSize().X;H=B->HasField(TEXT("height"))?B->GetNumberField(TEXT("height")):S->GetSize().Y;S->SetSize(FVector2D(W,H));}const TArray<TSharedPtr<FJsonValue>>*A=nullptr;if(B->TryGetArrayField(TEXT("anchors"),A)&&A->Num()==4)S->SetAnchors(FAnchors((*A)[0]->AsNumber(),(*A)[1]->AsNumber(),(*A)[2]->AsNumber(),(*A)[3]->AsNumber()));}
    }
    else if(Op==TEXT("remove")){if(Existing==BP->WidgetTree->RootWidget)BP->WidgetTree->RootWidget=nullptr;else BP->WidgetTree->RemoveWidget(Existing);}
    else if(Op==TEXT("reparent")){UPanelWidget* NewParent=Cast<UPanelWidget>(ParentWidget);if(!NewParent||NewParent==Existing){Tx.Cancel();OnComplete(ErrorResponse(TEXT("New parent must be a different panel"),EHttpServerResponseCodes::BadRequest));return true;}BP->WidgetTree->RemoveWidget(Existing);NewParent->AddChild(Existing);}
    else if(Op==TEXT("set_variable")){bool Value=true;B->TryGetBoolField(TEXT("value"),Value);Existing->Modify();Existing->bIsVariable=Value;}
    else if(Op==TEXT("set_slot")){UCanvasPanelSlot* S=Cast<UCanvasPanelSlot>(Existing->Slot);if(!S){Tx.Cancel();OnComplete(ErrorResponse(TEXT("Widget is not in a CanvasPanelSlot"),EHttpServerResponseCodes::BadRequest));return true;}S->Modify();double X=0,Y=0,W=100,H=30,AX=0,AY=0;int32 Z=0;bool Auto=false;B->TryGetNumberField(TEXT("x"),X);B->TryGetNumberField(TEXT("y"),Y);B->TryGetNumberField(TEXT("width"),W);B->TryGetNumberField(TEXT("height"),H);B->TryGetNumberField(TEXT("alignment_x"),AX);B->TryGetNumberField(TEXT("alignment_y"),AY);B->TryGetNumberField(TEXT("z_order"),Z);B->TryGetBoolField(TEXT("auto_size"),Auto);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(W,H));S->SetAlignment(FVector2D(AX,AY));S->SetAutoSize(Auto);S->SetZOrder(Z);const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(B->TryGetArrayField(TEXT("anchors"),A)&&A->Num()==4)S->SetAnchors(FAnchors((*A)[0]->AsNumber(),(*A)[1]->AsNumber(),(*A)[2]->AsNumber(),(*A)[3]->AsNumber()));}
    else if(Op==TEXT("set_property")){FString PropertyPath,Value;B->TryGetStringField(TEXT("property_path"),PropertyPath);B->TryGetStringField(TEXT("value"),Value);UObject* Owner=nullptr;FProperty* Property=nullptr;void* ValuePtr=nullptr;if(!ResolvePropertyPath(Existing,PropertyPath,Owner,Property,ValuePtr,Error)||!Property->ImportText_Direct(*Value,ValuePtr,Owner,PPF_None)){Tx.Cancel();OnComplete(ErrorResponse(Error.IsEmpty()?TEXT("Could not import property value"):Error,EHttpServerResponseCodes::BadRequest));return true;}}
    else if(Op==TEXT("set_brush")){FString TexturePath;B->TryGetStringField(TEXT("texture_path"),TexturePath);UTexture2D* T=LoadObject<UTexture2D>(nullptr,*TexturePath);UImage* I=Cast<UImage>(Existing);if(!T||!I){Tx.Cancel();OnComplete(ErrorResponse(TEXT("set_brush requires an Image and Texture2D"),EHttpServerResponseCodes::BadRequest));return true;}bool Match=false;B->TryGetBoolField(TEXT("match_size"),Match);I->SetBrushFromTexture(T,Match);}
    else if(Op==TEXT("audit")){}
    else{Tx.Cancel();OnComplete(ErrorResponse(TEXT("Unsupported widget-tree operation"),EHttpServerResponseCodes::BadRequest));return true;}
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);if(!SaveBlueprint(BP,Error)){Tx.Cancel();if(FPaths::FileExists(Snapshot))IFileManager::Get().Copy(*Source,*Snapshot,true,true);OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("operation"),Op);R->SetStringField(TEXT("widget_name"),Name);R->SetStringField(TEXT("snapshot_path"),Snapshot);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleImportAsset(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString Error;if(!ParseJsonBody(Request,B,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}FString Filename,Destination;B->TryGetStringField(TEXT("filename"),Filename);B->TryGetStringField(TEXT("destination_path"),Destination);bool Dry=true,UI=true,Replace=false;B->TryGetBoolField(TEXT("dry_run"),Dry);B->TryGetBoolField(TEXT("ui_texture"),UI);B->TryGetBoolField(TEXT("replace_existing"),Replace);const FString Ext=FPaths::GetExtension(Filename).ToLower();if(!FPaths::FileExists(Filename)||!Destination.StartsWith(TEXT("/Game"))||!(Ext==TEXT("png")||Ext==TEXT("jpg")||Ext==TEXT("jpeg")||Ext==TEXT("tga")||Ext==TEXT("svg")||Ext==TEXT("ttf")||Ext==TEXT("otf"))){OnComplete(ErrorResponse(TEXT("Valid source file and /Game destination are required"),EHttpServerResponseCodes::BadRequest));return true;}const FString AssetName=FPaths::GetBaseFilename(Filename);const FString ObjectPath=Destination/TEXT("")+AssetName+TEXT(".")+AssetName;if(LoadObject<UObject>(nullptr,*ObjectPath)&&!Replace){OnComplete(ErrorResponse(TEXT("Destination asset already exists"),EHttpServerResponseCodes::Conflict));return true;}if(Dry){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("source"),Filename);R->SetStringField(TEXT("expected_object_path"),ObjectPath);OnComplete(JsonResponse(R));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}UAutomatedAssetImportData* Data=NewObject<UAutomatedAssetImportData>();Data->Filenames={Filename};Data->DestinationPath=Destination;Data->bReplaceExisting=Replace;TArray<UObject*> Imported=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetsAutomated(Data);if(Imported.Num()==0){OnComplete(ErrorResponse(TEXT("Unreal import failed"),EHttpServerResponseCodes::Conflict));return true;}for(UObject* O:Imported)if(UTexture2D* T=Cast<UTexture2D>(O)){if(UI){T->Modify();T->LODGroup=TEXTUREGROUP_UI;T->CompressionSettings=TC_EditorIcon;T->MipGenSettings=TMGS_NoMipmaps;T->Filter=TF_Bilinear;T->SRGB=true;}T->PostEditChange();T->MarkPackageDirty();UPackage::SavePackage(T->GetOutermost(),T,*FPackageName::LongPackageNameToFilename(T->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),FSavePackageArgs());}const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);TArray<TSharedPtr<FJsonValue>> Paths;for(UObject* O:Imported)Paths.Add(MakeShared<FJsonValueString>(O->GetPathName()));R->SetArrayField(TEXT("object_paths"),Paths);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleListAssets(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    const FString Search=QueryValue(Request,TEXT("search")),Class=QueryValue(Request,TEXT("class"));int32 Limit=FMath::Clamp(FCString::Atoi(*QueryValue(Request,TEXT("limit"))),1,1000);if(Limit<=1)Limit=100;FAssetRegistryModule& M=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));TArray<FAssetData> Assets;M.Get().GetAllAssets(Assets,true);TArray<TSharedPtr<FJsonValue>> Out;for(const FAssetData&A:Assets){const FString P=A.GetObjectPathString(),C=A.AssetClassPath.ToString();if(!Search.IsEmpty()&&!P.Contains(Search,ESearchCase::IgnoreCase))continue;if(!Class.IsEmpty()&&!C.Contains(Class,ESearchCase::IgnoreCase))continue;const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),A.AssetName.ToString());J->SetStringField(TEXT("path"),P);J->SetStringField(TEXT("class"),C);Out.Add(MakeShared<FJsonValueObject>(J));if(Out.Num()>=Limit)break;}const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetNumberField(TEXT("returned"),Out.Num());R->SetArrayField(TEXT("assets"),Out);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleCapturePIE(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString Error;if(!ParseJsonBody(Request,B,Error)){OnComplete(ErrorResponse(Error,EHttpServerResponseCodes::BadRequest));return true;}if(!GEditor||!GEditor->PlayWorld){OnComplete(ErrorResponse(TEXT("PIE must be running"),EHttpServerResponseCodes::Conflict));return true;}FString Name=TEXT("CodexPIE");B->TryGetStringField(TEXT("name"),Name);Name=FPaths::MakeValidFileName(Name);const FString Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/(Name+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,false,false);const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("capture_requested"),true);R->SetStringField(TEXT("path"),Path);OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleCreateWidgetBlueprint(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString E;if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}FString PackageName,ParentPath=TEXT("/Script/UMG.UserWidget");B->TryGetStringField(TEXT("package_name"),PackageName);B->TryGetStringField(TEXT("parent_class"),ParentPath);bool Dry=true;B->TryGetBoolField(TEXT("dry_run"),Dry);const FString Name=FPackageName::GetLongPackageAssetName(PackageName),Folder=FPackageName::GetLongPackagePath(PackageName),ObjectPath=PackageName+TEXT(".")+Name;UClass* Parent=LoadObject<UClass>(nullptr,*ParentPath);if(!PackageName.StartsWith(TEXT("/Game/"))||Name.IsEmpty()||!Parent||!Parent->IsChildOf(UUserWidget::StaticClass())){OnComplete(ErrorResponse(TEXT("A new /Game package and UserWidget parent class are required"),EHttpServerResponseCodes::BadRequest));return true;}if(LoadObject<UObject>(nullptr,*ObjectPath)){OnComplete(ErrorResponse(TEXT("Destination asset already exists"),EHttpServerResponseCodes::Conflict));return true;}if(Dry){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),true);R->SetStringField(TEXT("object_path"),ObjectPath);R->SetStringField(TEXT("asset_class"),TEXT("/Script/UMGEditor.WidgetBlueprint"));OnComplete(JsonResponse(R));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}UWidgetBlueprintFactory* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=Parent;UObject* Created=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(Name,Folder,UWidgetBlueprint::StaticClass(),Factory);UWidgetBlueprint* BP=Cast<UWidgetBlueprint>(Created);if(!BP||!SaveBlueprint(BP,E)){OnComplete(ErrorResponse(E.IsEmpty()?TEXT("Widget Blueprint creation failed"):E,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("object_path"),BP->GetPathName());R->SetStringField(TEXT("asset_class"),BP->GetClass()->GetPathName());OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleVariableMetadata(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString E;if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}FString Path,Name,NewName,Category,Tooltip,RepNotify;B->TryGetStringField(TEXT("blueprint_path"),Path);B->TryGetStringField(TEXT("variable_name"),Name);B->TryGetStringField(TEXT("new_name"),NewName);B->TryGetStringField(TEXT("category"),Category);B->TryGetStringField(TEXT("tooltip"),Tooltip);B->TryGetStringField(TEXT("rep_notify"),RepNotify);bool Dry=true;B->TryGetBoolField(TEXT("dry_run"),Dry);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);FBPVariableDescription* Var=BP?BP->NewVariables.FindByPredicate([&](const FBPVariableDescription&V){return V.VarName==*Name;}):nullptr;if(!Var){OnComplete(ErrorResponse(TEXT("Blueprint member variable not found"),EHttpServerResponseCodes::NotFound));return true;}const auto Preview=MakeShared<FJsonObject>();Preview->SetBoolField(TEXT("ok"),true);Preview->SetBoolField(TEXT("dry_run"),Dry);Preview->SetStringField(TEXT("variable_name"),Name);Preview->SetStringField(TEXT("category"),Category);Preview->SetStringField(TEXT("new_name"),NewName);if(Dry){OnComplete(JsonResponse(Preview));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","VariableMetadata","Codex: Edit variable metadata"));BP->Modify();if(!Category.IsEmpty())FBlueprintEditorUtils::SetBlueprintVariableCategory(BP,*Name,nullptr,FText::FromString(Category),true);if(!Tooltip.IsEmpty())FBlueprintEditorUtils::SetBlueprintVariableMetaData(BP,*Name,nullptr,TEXT("Tooltip"),Tooltip);auto SetFlag=[&](const TCHAR*Field,uint64 Flag,bool Invert=false){bool V;if(B->TryGetBoolField(Field,V)){if(Invert)V=!V;if(V)Var->PropertyFlags|=Flag;else Var->PropertyFlags&=~Flag;}};SetFlag(TEXT("instance_editable"),CPF_Edit);SetFlag(TEXT("expose_on_spawn"),CPF_ExposeOnSpawn);SetFlag(TEXT("private"),CPF_BlueprintVisible,true);SetFlag(TEXT("save_game"),CPF_SaveGame);SetFlag(TEXT("replicated"),CPF_Net);if(!RepNotify.IsEmpty()){Var->RepNotifyFunc=*RepNotify;Var->PropertyFlags|=CPF_RepNotify|CPF_Net;}if(!NewName.IsEmpty()&&!NewName.Equals(Name)){FBlueprintEditorUtils::RenameMemberVariable(BP,*Name,*NewName);Name=NewName;}FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);if(!SaveBlueprint(BP,E)){Tx.Cancel();OnComplete(ErrorResponse(E,EHttpServerResponseCodes::Conflict));return true;}LastEditedBlueprint=BP;Preview->SetBoolField(TEXT("dry_run"),false);Preview->SetStringField(TEXT("variable_name"),Name);OnComplete(JsonResponse(Preview));return true;
}

bool FCodexBlueprintBridgeModule::HandleStalePins(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    FString Path,GraphName,NodeId,PinId,Op=TEXT("analyze");TSharedPtr<FJsonObject>B;FString E;bool Dry=true;if(Request.Verb==EHttpServerRequestVerbs::VERB_GET){Path=QueryValue(Request,TEXT("path"));GraphName=QueryValue(Request,TEXT("graph"));}else{if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}B->TryGetStringField(TEXT("blueprint_path"),Path);B->TryGetStringField(TEXT("graph_name"),GraphName);B->TryGetStringField(TEXT("node_id"),NodeId);B->TryGetStringField(TEXT("pin_id"),PinId);B->TryGetStringField(TEXT("operation"),Op);B->TryGetBoolField(TEXT("dry_run"),Dry);}UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);if(!BP){OnComplete(ErrorResponse(TEXT("Blueprint not found; use the full object path /Game/Path/Asset.Asset"),EHttpServerResponseCodes::NotFound));return true;}TArray<UEdGraph*> Graphs;if(GraphName.IsEmpty())BP->GetAllGraphs(Graphs);else if(UEdGraph*G=FindBlueprintGraph(BP,GraphName))Graphs.Add(G);if(Graphs.Num()==0){OnComplete(ErrorResponse(TEXT("Blueprint graph not found"),EHttpServerResponseCodes::NotFound));return true;}TArray<TSharedPtr<FJsonValue>> Issues;UEdGraphNode* Target=nullptr;UEdGraphPin* TargetPin=nullptr;for(UEdGraph*G:Graphs)for(UEdGraphNode*N:G->Nodes)if(N){if(N->NodeGuid.ToString().Equals(NodeId,ESearchCase::IgnoreCase))Target=N;for(UEdGraphPin*P:N->Pins)if(P){if(P->PinId.ToString().Equals(PinId,ESearchCase::IgnoreCase))TargetPin=P;FString Reason;if(P->bOrphanedPin)Reason=TEXT("orphaned_pin");else if(P->ParentPin&&!P->ParentPin->SubPins.Contains(P))Reason=TEXT("split_pin_parent_mismatch");else if(P->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard&&P->LinkedTo.Num()>0)Reason=TEXT("connected_wildcard");if(!Reason.IsEmpty()){const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("graph"),G->GetName());J->SetStringField(TEXT("node_id"),N->NodeGuid.ToString());J->SetStringField(TEXT("node_title"),N->GetNodeTitle(ENodeTitleType::ListView).ToString());J->SetStringField(TEXT("pin_id"),P->PinId.ToString());J->SetStringField(TEXT("pin_name"),P->PinName.ToString());J->SetStringField(TEXT("reason"),Reason);Issues.Add(MakeShared<FJsonValueObject>(J));}}}if(Op==TEXT("analyze")||Request.Verb==EHttpServerRequestVerbs::VERB_GET||Dry){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetBoolField(TEXT("dry_run"),Dry);R->SetNumberField(TEXT("issue_count"),Issues.Num());R->SetBoolField(TEXT("safe_to_compile"),Issues.Num()==0);R->SetArrayField(TEXT("issues"),Issues);OnComplete(JsonResponse(R));return true;}if(!bEditModeEnabled){OnComplete(ErrorResponse(TEXT("Edit mode is locked"),EHttpServerResponseCodes::Forbidden));return true;}if(Op==TEXT("reconstruct_node")&&!Target){OnComplete(ErrorResponse(TEXT("node_id not found"),EHttpServerResponseCodes::NotFound));return true;}if(Op==TEXT("recombine_pin")&&!TargetPin){OnComplete(ErrorResponse(TEXT("pin_id not found"),EHttpServerResponseCodes::NotFound));return true;}FScopedTransaction Tx(NSLOCTEXT("CodexBlueprintBridge","RepairPins","Codex: Repair stale pins"));BP->Modify();if(Op==TEXT("reconstruct_node")){Target->Modify();Target->ReconstructNode();}else if(Op==TEXT("recombine_pin")){TargetPin->GetOwningNode()->Modify();GetDefault<UEdGraphSchema_K2>()->RecombinePin(TargetPin);}else{OnComplete(ErrorResponse(TEXT("operation must be analyze, reconstruct_node, or recombine_pin"),EHttpServerResponseCodes::BadRequest));return true;}FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);BP->GetOutermost()->MarkPackageDirty();const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),true);R->SetStringField(TEXT("operation"),Op);R->SetBoolField(TEXT("compiled"),false);R->SetStringField(TEXT("next_step"),TEXT("Run protected compile after re-analysis"));OnComplete(JsonResponse(R));return true;
}

bool FCodexBlueprintBridgeModule::HandleProtectedCompile(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete)
{
    TSharedPtr<FJsonObject>B;FString E;if(!ParseJsonBody(Request,B,E)){OnComplete(ErrorResponse(E,EHttpServerResponseCodes::BadRequest));return true;}FString Path;B->TryGetStringField(TEXT("blueprint_path"),Path);bool Allow=false;B->TryGetBoolField(TEXT("allow_suspicious"),Allow);UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path);if(!BP){OnComplete(ErrorResponse(TEXT("Blueprint not found; use the full object path /Game/Path/Asset.Asset"),EHttpServerResponseCodes::NotFound));return true;}TArray<TSharedPtr<FJsonValue>> Issues;TArray<UEdGraph*>Graphs;BP->GetAllGraphs(Graphs);for(UEdGraph*G:Graphs)for(UEdGraphNode*N:G->Nodes)if(N)for(UEdGraphPin*P:N->Pins)if(P&&(P->bOrphanedPin||(P->ParentPin&&!P->ParentPin->SubPins.Contains(P))||(P->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard&&P->LinkedTo.Num()>0))){const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("graph"),G->GetName());J->SetStringField(TEXT("node"),N->GetNodeTitle(ENodeTitleType::ListView).ToString());J->SetStringField(TEXT("pin"),P->PinName.ToString());Issues.Add(MakeShared<FJsonValueObject>(J));}if(Issues.Num()>0&&!Allow){const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),false);R->SetBoolField(TEXT("compile_blocked"),true);R->SetArrayField(TEXT("suspicious_pins"),Issues);R->SetStringField(TEXT("error"),TEXT("Compilation blocked before invoking the Blueprint compiler"));OnComplete(JsonResponse(R,EHttpServerResponseCodes::Conflict));return true;}const FString Dir=FPaths::ProjectSavedDir()/TEXT("CodexBlueprintBridge/PreCompile");IFileManager::Get().MakeDirectory(*Dir,true);const FString Source=FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());const FString Snapshot=Dir/(BP->GetName()+TEXT("_")+FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S%f"))+TEXT(".uasset"));if(FPaths::FileExists(Source))IFileManager::Get().Copy(*Snapshot,*Source,true,true);FCompilerResultsLog Log;Log.bSilentMode=true;Log.bLogDetailedResults=true;FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Log);TArray<TSharedPtr<FJsonValue>>Messages;for(const auto&M:Log.Messages){const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("severity"),FTokenizedMessage::GetSeverityText(M->GetSeverity()).ToString());J->SetStringField(TEXT("message"),M->ToText().ToString());Messages.Add(MakeShared<FJsonValueObject>(J));}const auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("ok"),Log.NumErrors==0);R->SetNumberField(TEXT("errors"),Log.NumErrors);R->SetNumberField(TEXT("warnings"),Log.NumWarnings);R->SetStringField(TEXT("precompile_snapshot"),Snapshot);R->SetArrayField(TEXT("messages"),Messages);OnComplete(JsonResponse(R,Log.NumErrors==0?EHttpServerResponseCodes::Ok:EHttpServerResponseCodes::Conflict));return true;
}

IMPLEMENT_MODULE(FCodexBlueprintBridgeModule, CodexBlueprintBridge)
