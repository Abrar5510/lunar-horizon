#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LHTypes.h"
#include "LHEphemerisSubsystem.generated.h"

// SPICE sky for any site and UTC. Loads Content/SPICE kernels and Content/Data/Sites.json.
UCLASS()
class LHSCIENCE_API ULHEphemerisSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ~10 SPICE calls, well under a millisecond: fine to call every frame
	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	FLHSkyState GetSkyState(const FLHSite& Site, FDateTime Utc);

	UFUNCTION(BlueprintPure, Category = "LunarHorizon")
	bool IsReady() const { return bReady; }

	UFUNCTION(BlueprintPure, Category = "LunarHorizon")
	TArray<FLHSite> GetSites() const { return Sites; }

	// v_moonme = R * v_j2000, for turning the star sphere
	bool J2000ToMoonMe(FDateTime Utc, double OutR[3][3]) const;

	static bool InitSpice(FString& OutError);   // also used by the automation tests
	static double UtcToEt(FDateTime Utc);

private:
	bool bReady = false;
	UPROPERTY() TArray<FLHSite> Sites;
};
