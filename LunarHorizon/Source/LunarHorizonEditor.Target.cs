using UnrealBuildTool;

public class LunarHorizonEditorTarget : TargetRules
{
	public LunarHorizonEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new[] { "LunarHorizon", "LHScience" });
	}
}
