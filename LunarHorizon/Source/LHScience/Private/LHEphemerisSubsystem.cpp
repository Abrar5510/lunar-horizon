#include "LHEphemerisSubsystem.h"
#include "LHHorizonSubsystem.h"
#include "LHSpice.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogLHScience, Log, All);

bool ULHEphemerisSubsystem::InitSpice(FString& OutError)
{
	// Absolute paths: kernels are staged loose (non-UFS) next to the .pak in packaged builds
	const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("SPICE"));
	std::vector<std::string> Paths;
	for (const char* K : LHCore::Kernels) Paths.push_back(TCHAR_TO_UTF8(*(Dir / UTF8_TO_TCHAR(K))));
	std::string Err;
	const bool bOk = FLHSpice::Init(Paths, Err);
	OutError = UTF8_TO_TCHAR(Err.c_str());
	return bOk;
}

double ULHEphemerisSubsystem::UtcToEt(FDateTime Utc)
{
	double Et = 0;
	FLHSpice::UtcToEt(TCHAR_TO_UTF8(*Utc.ToString(TEXT("%Y-%m-%dT%H:%M:%S.%s"))), Et);
	return Et;
}

void ULHEphemerisSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<ULHHorizonSubsystem>();

	FString Err;
	bReady = InitSpice(Err);
	if (!bReady) UE_LOG(LogLHScience, Error, TEXT("SPICE init failed: %s"), *Err);

	FString Json;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/Sites.json");
	if (!FFileHelper::LoadFileToString(Json, *Path) || !FJsonObjectConverter::JsonArrayStringToUStruct(Json, &Sites, 0, 0))
	{
		UE_LOG(LogLHScience, Error, TEXT("Could not read %s"), *Path);
	}
}

FLHSkyState ULHEphemerisSubsystem::GetSkyState(const FLHSite& Site, FDateTime Utc)
{
	FLHSkyState S;
	if (!bReady) return S;

	const double Et = UtcToEt(Utc);
	double Obs[3], SunDir[3], EarthDir[3];
	LHCore::SiteVectorKm(Site.LatDeg, Site.LonDeg, Site.HeightM, Obs);
	const FLHHorizonMask* Mask = GetGameInstance()->GetSubsystem<ULHHorizonSubsystem>()->Find(Site.Id);
	FLHSky Sky;
	if (!FLHSpice::Sky(Et, Obs, Mask ? Mask->ElevationDeg.GetData() : nullptr, Mask ? Mask->ElevationDeg.Num() : 0, Sky)
		|| !FLHSpice::DirMoonMe("SUN", Et, Obs, SunDir) || !FLHSpice::DirMoonMe("EARTH", Et, Obs, EarthDir))
	{
		UE_LOG(LogLHScience, Warning, TEXT("SPICE: %s"), UTF8_TO_TCHAR(FLHSpice::LastError().c_str()));
		return S;
	}
	const double Alpha = FLHSpice::EarthPhaseAngleRad(Et);

	S.SunAzDeg = Sky.Sun.AzDeg;
	S.SunElDeg = Sky.Sun.ElDeg;
	S.SunDistanceKm = Sky.Sun.RangeKm;
	S.EarthAzDeg = Sky.Earth.AzDeg;
	S.EarthElDeg = Sky.Earth.ElDeg;
	S.EarthDistanceKm = Sky.Earth.RangeKm;
	S.SunVisibleFraction = float(Sky.SunFrac);
	S.EarthVisible = Sky.EarthVis;
	S.EarthPhase = float((1.0 + FMath::Cos(Alpha)) / 2.0);
	S.EarthshineLux = Sky.EarthVis ? float(LHCore::EarthshineLux(Alpha, Sky.Earth.RangeKm)) : 0.f;
	S.DSNVisible = FLHSpice::AnyDsnSeesMoon(Et);
	S.SunDirMoonMe = FVector(SunDir[0], SunDir[1], SunDir[2]);
	S.EarthDirMoonMe = FVector(EarthDir[0], EarthDir[1], EarthDir[2]);
	return S;
}

bool ULHEphemerisSubsystem::J2000ToMoonMe(FDateTime Utc, double OutR[3][3]) const
{
	return bReady && FLHSpice::Rotation("J2000", "MOON_ME", UtcToEt(Utc), OutR);
}
