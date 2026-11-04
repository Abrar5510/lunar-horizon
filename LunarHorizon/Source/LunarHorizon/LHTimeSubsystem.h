#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LHTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLHOnTimeChanged, FDateTime, Utc);

// Simulated UTC. Science always derives ET from this absolute time, never from accumulated deltas.
UCLASS()
class LUNARHORIZON_API ULHTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override
	{
		if (!bPlaying) return;
		SimUtc += FTimespan::FromSeconds(double(DeltaTime) * PlaybackRate);
		OnTimeChanged.Broadcast(SimUtc);
	}
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULHTimeSubsystem, STATGROUP_Tickables); }

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetTime(FDateTime Utc) { SimUtc = Utc; OnTimeChanged.Broadcast(SimUtc); }

	// Presets: 1 (real time), 3600 (1 h/s), 86400 (1 d/s), 85000 (a month in 30 s)
	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetPlaybackRate(double SimSecondsPerSecond) { PlaybackRate = SimSecondsPerSecond; }

	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetPlaying(bool bInPlaying) { bPlaying = bInPlaying; }

	UFUNCTION(BlueprintPure, Category = "LunarHorizon")
	FDateTime GetTime() const { return SimUtc; }

	UFUNCTION(BlueprintPure, Category = "LunarHorizon")
	bool IsPlaying() const { return bPlaying; }

	UPROPERTY(BlueprintAssignable, Category = "LunarHorizon")
	FLHOnTimeChanged OnTimeChanged;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}

private:
	FDateTime SimUtc = FDateTime(2026, 11, 14, 18, 0, 0);   // hackathon weekend
	double PlaybackRate = 1.0;
	bool bPlaying = false;
};
