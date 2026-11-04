#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LHTypes.h"
#include "LHTimelineWidget.generated.h"

// Gantt rows Sunlit (>= 50 % disk) / Earth visible / DSN / DTE over the sweep window, a "now"
// line, and click-drag to scrub time (docs/01 section 7.4). Base class for WBP_Timeline.
UCLASS()
class LUNARHORIZON_API ULHTimelineWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetSweep(const FLHSweepResult& InSweep) { Sweep = InSweep; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply NativeOnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override;

private:
	void ScrubTo(const FGeometry& G, const FPointerEvent& E) const;
	FTimespan Window() const { return FTimespan::FromMinutes(Sweep.SunFraction.Num() * double(Sweep.StepMinutes)); }

	FLHSweepResult Sweep;
	static constexpr float LabelW = 64.f;
};
