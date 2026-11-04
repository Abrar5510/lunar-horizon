#include "LHEphemerisSubsystem.h"
#include "LHSpice.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

// pole_nov.csv: python tools/lunar_sky.py --kernels LunarHorizon/Content/SPICE --site -89.9 0 \
//   --start 2026-11-01T00:00:00 --days 30 --csv LunarHorizon/Source/LHScience/Private/Tests/pole_nov.csv
// Headless: UnrealEditor-Cmd LunarHorizon.uproject -ExecCmds="Automation RunTests LunarHorizon; Quit" -unattended -nullrhi
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLHEphemerisMatchesPython, "LunarHorizon.Science.EphemerisVsPython",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLHEphemerisMatchesPython::RunTest(const FString&)
{
	FString Err;
	const bool bInit = ULHEphemerisSubsystem::InitSpice(Err);
	if (!TestTrue(TEXT("SPICE init: ") + Err, bInit)) return false;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectDir() / TEXT("Source/LHScience/Private/Tests/pole_nov.csv")));
	if (!TestTrue(TEXT("oracle CSV"), Lines.Num() > 1)) return false;

	double Obs[3];
	LHCore::SiteVectorKm(-89.9, 0.0, 0.0, Obs);
	for (int32 i = 1; i < Lines.Num(); i += 48)   // one row per simulated day
	{
		TArray<FString> C;   // utc,sun_az,sun_el,sun_km,sun_frac,earth_az,earth_el,...
		Lines[i].ParseIntoArray(C, TEXT(","));
		FDateTime Utc;
		FDateTime::ParseIso8601(*C[0], Utc);
		FLHSky Sky;
		TestTrue(TEXT("SPICE sky"), FLHSpice::Sky(ULHEphemerisSubsystem::UtcToEt(Utc), Obs, nullptr, 0, Sky));
		TestNearlyEqual(TEXT("Sun az"), Sky.Sun.AzDeg, FCString::Atod(*C[1]), 1e-4);
		TestNearlyEqual(TEXT("Sun el"), Sky.Sun.ElDeg, FCString::Atod(*C[2]), 1e-4);
		TestNearlyEqual(TEXT("Earth el"), Sky.Earth.ElDeg, FCString::Atod(*C[6]), 1e-4);
	}
	return true;
}

#endif
