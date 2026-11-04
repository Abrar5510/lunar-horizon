using UnrealBuildTool;

public class LunarHorizonTarget : TargetRules
{
	public LunarHorizonTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new[] { "LunarHorizon", "LHScience" });
	}
}
