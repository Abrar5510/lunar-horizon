using System.IO;
using UnrealBuildTool;

// NAIF CSPICE N0067 as a prebuilt static lib. Mac: universal arm64 + x86_64 built from the MacM1
// package (its headers already define CSPICE_MAC_OSX_M1_64BIT_CLANG). Win64: rebuild with /MD (docs/04 section 4.1).
public class CSPICE : ModuleRules
{
	public CSPICE(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "Win64", "cspice.lib"));
			PublicDefinitions.AddRange(new[] { "MSDOS=1", "OMIT_BLANK_CC=1", "NON_ANSI_STDIO=1" });
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "Mac", "libcspice.a"));
		}
	}
}
