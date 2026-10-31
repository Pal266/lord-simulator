using UnrealBuildTool;

public class LordSimulatorTests : ModuleRules
{
	public LordSimulatorTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "LordSimulator", "UMG", "CommonUI", "CommonInput",
			"Slate", "SlateCore", "UnrealEd"
		});
	}
}
