#pragma once
#include "CoreMinimal.h"
#include "LHCore.h"
#include "LHTypes.generated.h"

UENUM(BlueprintType)
enum class ELHSiteKind : uint8 { ArtemisRegion, CLPSMission, Reference, Custom };

// One entry of Content/Data/Sites.json (same field names)
USTRUCT(BlueprintType)
struct LHSCIENCE_API FLHSite
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") double LatDeg = -89.9;   // MOON_ME
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") double LonDeg = 0.0;     // east-positive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") double HeightM = 0.0;    // above the 1737.4 km sphere
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") ELHSiteKind Kind = ELHSiteKind::Custom;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") FString Region;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") FString Source;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon") FString Blurb;
};

USTRUCT(BlueprintType)
struct LHSCIENCE_API FLHSkyState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double SunAzDeg = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double SunElDeg = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double EarthAzDeg = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double EarthElDeg = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float SunVisibleFraction = 0;   // 0..1 of the solar disk above the terrain
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") bool EarthVisible = false;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float EarthPhase = 0;           // illuminated fraction 0..1
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float EarthshineLux = 0;        // reaching the site, 0 when Earth is hidden
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") bool DSNVisible = false;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double SunDistanceKm = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") double EarthDistanceKm = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FVector SunDirMoonMe = FVector::ZeroVector;    // unit, body-fixed
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FVector EarthDirMoonMe = FVector::ZeroVector;
};

// Terrain elevation angle for equal azimuth bins starting at 0 deg (north), clockwise
USTRUCT(BlueprintType)
struct LHSCIENCE_API FLHHorizonMask
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float AzimuthStepDeg = 0.25f;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> ElevationDeg;

	double At(double AzDeg) const { return LHCore::MaskAt(ElevationDeg.GetData(), ElevationDeg.Num(), AzDeg); }
};

USTRUCT(BlueprintType)
struct LHSCIENCE_API FLHSiteMetrics
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float IlluminationPct = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float LongestDarkHours = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float DTEPct = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float DSNCoveragePct = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float AvgPowerW = 0;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") bool bHasNextSunrise = false;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FDateTime NextSunriseUtc;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") bool bHasNextEarthrise = false;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FDateTime NextEarthriseUtc;
};

// One site over one analysis window: metrics plus per-step series for the Timeline and Panorama
USTRUCT(BlueprintType)
struct LHSCIENCE_API FLHSweepResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FLHSite Site;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FLHSiteMetrics Metrics;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") FDateTime StartUtc;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") float StepMinutes = 30;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> SunFraction;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<bool> EarthVisible;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<bool> DSN;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> SunAz;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> SunEl;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> EarthAz;
	UPROPERTY(BlueprintReadOnly, Category = "LunarHorizon") TArray<float> EarthEl;
};
