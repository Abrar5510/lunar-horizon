using UnrealBuildTool;

public class LHScience : ModuleRules
{
	public LHScience(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "CSPICE", "Json", "JsonUtilities" });
	}
}
