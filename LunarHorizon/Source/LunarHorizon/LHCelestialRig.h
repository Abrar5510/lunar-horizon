#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LHTypes.h"
#include "LHCelestialRig.generated.h"

class ACesiumGeoreference;
class UDirectionalLightComponent;
class UMaterialParameterCollection;
class UStaticMeshComponent;

// Sun, earthshine, Earth disk and star sphere, driven every frame by SPICE at the simulated UTC.
// Directions are computed in MOON_ME and rotated into Unreal by the georeference, so they are
// right anywhere on the globe. Without a georeference the level is East-South-Up at the site.
UCLASS()
class LUNARHORIZON_API ALHCelestialRig : public AActor
{
	GENERATED_BODY()
public:
	ALHCelestialRig();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetSite(const FLHSite& InSite) { Site = InSite; }

	UFUNCTION(BlueprintPure, Category = "LunarHorizon")
	FLHSkyState GetSkyState() const { return Sky; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon")
	FLHSite Site;

	UPROPERTY(EditAnywhere, Category = "LunarHorizon")
	TObjectPtr<ACesiumGeoreference> Georef;

	// MPC_Lunar: SunDirWS, EarthDirWS (vectors), SunVisible, EarthPhase (scalars)
	UPROPERTY(EditAnywhere, Category = "LunarHorizon")
	TObjectPtr<UMaterialParameterCollection> MPC;

	UPROPERTY(VisibleAnywhere, Category = "LunarHorizon")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "LunarHorizon")
	TObjectPtr<UDirectionalLightComponent> Earthshine;

	UPROPERTY(VisibleAnywhere, Category = "LunarHorizon")
	TObjectPtr<UStaticMeshComponent> EarthMesh;

	// Star map convention for M_Stars: sphere local (x, -y, z) = J2000 (x, y, z), the same
	// handedness flip Cesium uses between ECEF and Unreal.
	UPROPERTY(VisibleAnywhere, Category = "LunarHorizon")
	TObjectPtr<UStaticMeshComponent> StarSphere;

private:
	FVector ToUnreal(const FVector& DirMoonMe) const;
	FLHSkyState Sky;
};
