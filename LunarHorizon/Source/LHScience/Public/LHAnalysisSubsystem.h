#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LHTypes.h"
#include "LHAnalysisSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLHOnSweepDone, const TArray<FLHSweepResult>&, Results);

// Site comparison: N sites over one window, metrics identical to lunar_sky.metrics()
UCLASS()
class LHSCIENCE_API ULHAnalysisSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Runs on one worker thread (SPICE is serialized, more threads would not help) and
	// broadcasts OnSweepDone on the game thread. 5 sites x 30 days x 30 min is well under a second.
	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void RunSweepAsync(const TArray<FLHSite>& Sites, FDateTime StartUtc, float Days = 30.f, float StepMinutes = 30.f);

	UPROPERTY(BlueprintAssignable, Category = "LunarHorizon")
	FLHOnSweepDone OnSweepDone;
};
