using UnrealBuildTool;

public class LunarHorizon : ModuleRules
{
	public LunarHorizon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory); // UI/ and root headers include each other
		PublicDependencyModuleNames.AddRange(new[] {
			"Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore",
			"ProceduralMeshComponent", "CesiumRuntime", "LHScience" });
	}
}
