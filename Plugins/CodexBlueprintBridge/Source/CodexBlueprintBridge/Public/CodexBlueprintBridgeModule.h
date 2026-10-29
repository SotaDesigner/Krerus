#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"
#include "HttpResultCallback.h"
#include "Modules/ModuleManager.h"

class IHttpRouter;
struct FHttpServerRequest;
struct FHttpServerResponse;

class FCodexBlueprintBridgeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    bool HandleHealth(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleListBlueprints(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleInspectBlueprint(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleEditMode(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleCreateBlueprint(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleAddVariable(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleApplyGraphPatch(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleClassDefaults(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleSetDefaultProperty(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleEditGraph(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleUndo(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleEnhancedInput(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleSearchActions(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleDescribeAction(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleCompileBlueprint(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete);
    bool HandleDuplicateAsset(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleBlueprintFunction(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleBlueprintDispatcher(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleBlueprintInterface(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleBlueprintComponent(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandlePIE(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleSnapshot(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleInspectRegion(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleComponentDetails(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleAnalyzeExtractFunction(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleWidgetTree(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleImportAsset(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleListAssets(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleCapturePIE(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleCreateWidgetBlueprint(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleVariableMetadata(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleStalePins(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleProtectedCompile(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

    void StartServer();
    void StopServer();

    static constexpr uint32 Port = 32123;
    TSharedPtr<IHttpRouter> Router;
    TArray<FHttpRouteHandle> RouteHandles;
    bool bEditModeEnabled = false;
    TWeakObjectPtr<class UBlueprint> LastEditedBlueprint;
};
