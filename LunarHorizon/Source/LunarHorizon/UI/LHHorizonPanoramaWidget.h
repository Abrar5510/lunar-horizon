#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LHTypes.h"
#include "LHHorizonPanoramaWidget.generated.h"

// 360 deg skyline (docs/01 section 7.3): terrain silhouette, Sun/Earth tracks (solid where visible,
// faint where terrain blocks them) and live "now" markers. Base class for WBP_HorizonPanorama.
UCLASS()
class LUNARHORIZON_API ULHHorizonPanoramaWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Site, horizon mask and tracks from one sweep result
	UFUNCTION(BlueprintCallable, Category = "LunarHorizon")
	void SetSweep(const FLHSweepResult& Sweep);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon")
	float ElMin = -10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarHorizon")
	float ElMax = 15.f;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	FLHSite Site;
	FLHHorizonMask Mask;
	TArray<FVector2D> SunTrack, EarthTrack;   // (az, el)
};
