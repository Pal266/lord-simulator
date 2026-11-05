using UnrealBuildTool;

public class LordSimulatorEditor : ModuleRules
{
    public LordSimulatorEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "Landscape", "Water",
            "Json", "ImageWrapper", "AssetRegistry", "MaterialEditor", "Foliage", "GeometryCore",
            "LordSimulator", "UMG", "UMGEditor", "CommonUI", "SlateCore", "KismetCompiler"
        });
    }
}
