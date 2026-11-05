using UnrealBuildTool;

public class MarsMapEditorTarget : TargetRules
{
	public MarsMapEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MarsMap");
	}
}
