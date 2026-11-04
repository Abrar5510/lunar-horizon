#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LHTypes.h"
#include "LHHorizonSubsystem.generated.h"

// Precomputed terrain horizons from Content/Data/Horizons (tools/make_horizon_masks.py)
UCLASS()
class LHSCIENCE_API ULHHorizonSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// Key = site id (2 m eye) or id + "_10m" (lander mast). Null if there is no mask.
	// Game thread only; the pointer is valid until the next Find.
	const FLHHorizonMask* Find(const FString& Key);

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	bool GetMask(const FString& Key, FLHHorizonMask& Out);

private:
	TMap<FString, FLHHorizonMask> Masks;
};
