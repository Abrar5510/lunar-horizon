#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LHTypes.h"
#include "LHHorizonOverlay.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

// The "AR layer": terrain horizon ring plus Sun (amber) and Earth (blue) paths, painted on the
// sky at a fixed radius around the actor. Place it at the site (East-South-Up, like the georeference origin).
UCLASS()
class LUNARHORIZON_API ALHHorizonOverlay : public AActor
{
	GENERATED_BODY()
public:
	ALHHorizonOverlay();

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetMask(const FLHHorizonMask& Mask);

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetTracks(const FLHSweepResult& Sweep);

	UPROPERTY(EditAnywhere, Category = "LunarHorizon")
	float RadiusCm = 300000.f;   // 3 km

	UPROPERTY(EditAnywhere, Category = "LunarHorizon")
	float WidthCm = 1500.f;      // ~0.3 deg at 3 km

	// M_SkyTrack: unlit, translucent, two-sided, emissive = vertex color x 4
	UPROPERTY(EditAnywhere, Category = "LunarHorizon")
	TObjectPtr<UMaterialInterface> TrackMaterial;

	UPROPERTY(VisibleAnywhere, Category = "LunarHorizon")
	TObjectPtr<UProceduralMeshComponent> Mesh;

private:
	void BuildRibbon(int32 Section, const TArray<FVector>& Dirs, const FLinearColor& Color);
};
