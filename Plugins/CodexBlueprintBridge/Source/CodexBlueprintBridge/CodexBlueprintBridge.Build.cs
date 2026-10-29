using UnrealBuildTool;

public class CodexBlueprintBridge : ModuleRules
{
    public CodexBlueprintBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine"
            ,"EnhancedInput",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetRegistry",
            "AssetTools",
            "BlueprintGraph",
            "HTTPServer",
            "Json",
            "JsonUtilities",
            "Kismet",
            "InputBlueprintNodes",
            "InputCore",
            "UnrealEd"
            ,"UMGEditor",
            "Slate",
            "SlateCore"
        });
    }
}
