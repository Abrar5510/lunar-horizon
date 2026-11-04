#include "UI/LHHorizonPanoramaWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "LHEphemerisSubsystem.h"
#include "LHHorizonSubsystem.h"
#include "LHTimeSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

static const FLinearColor SunColor(1.0f, 0.66f, 0.10f), EarthColor(0.25f, 0.61f, 1.0f), Muted(0.55f, 0.57f, 0.60f);

void ULHHorizonPanoramaWidget::SetSweep(const FLHSweepResult& Sweep)
{
	Site = Sweep.Site;
	Mask = FLHHorizonMask();
	if (ULHHorizonSubsystem* Horizons = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULHHorizonSubsystem>() : nullptr)
	{
		Horizons->GetMask(Site.Id, Mask);
	}
	SunTrack.Reset();
	EarthTrack.Reset();
	for (int32 i = 0; i < Sweep.SunAz.Num(); ++i)
	{
		SunTrack.Add(FVector2D(Sweep.SunAz[i], Sweep.SunEl[i]));
		EarthTrack.Add(FVector2D(Sweep.EarthAz[i], Sweep.EarthEl[i]));
	}
}

int32 ULHHorizonPanoramaWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, G, Cull, Out, Layer, Style, bParentEnabled);
	const FVector2f Sz(G.GetLocalSize());
	auto ToPx = [&](double Az, double El) {
		return FVector2f(float(Az / 360.0) * Sz.X, float(1.0 - (El - ElMin) / (ElMax - ElMin)) * Sz.Y); };
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);

	// terrain silhouette: one column per bin, filled to the bottom
	const int32 Bins = Mask.ElevationDeg.Num();
	for (int32 i = 0; i < Bins; ++i)
	{
		const FVector2f Top = ToPx(i * Mask.AzimuthStepDeg, Mask.ElevationDeg[i]);
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Sz.X / Bins + 1.f, Sz.Y - Top.Y), FSlateLayoutTransform(Top)),
			White, ESlateDrawEffect::None, FLinearColor(0.10f, 0.10f, 0.11f));
	}

	// flat horizon (el = 0) and compass
	FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ToPx(0, 0), ToPx(360, 0)},
		ESlateDrawEffect::None, Muted, true, 1.f);
	const TCHAR* Compass[] = {TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W"), TEXT("N")};
	for (int32 k = 0; k < 5; ++k)
	{
		const float X = FMath::Clamp(k * Sz.X / 4.f - 4.f, 2.f, Sz.X - 12.f);
		FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(FVector2f(X, 2.f))),
			FString(Compass[k]), Font, ESlateDrawEffect::None, Muted);
	}

	// tracks: one polyline per run, split where the azimuth wraps or the terrain starts/stops blocking
	auto Track = [&](const TArray<FVector2D>& AzEl, const FLinearColor& Color)
	{
		TArray<FVector2f> Run;
		bool bRunVisible = false;
		auto Flush = [&]()
		{
			if (Run.Num() > 1)
			{
				FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), Run, ESlateDrawEffect::None,
					bRunVisible ? Color : Color.CopyWithNewOpacity(0.3f), true, 2.f);
			}
			Run.Reset();
		};
		for (int32 i = 0; i < AzEl.Num(); ++i)
		{
			const FVector2f P = ToPx(AzEl[i].X, AzEl[i].Y);
			const bool bVisible = AzEl[i].Y > Mask.At(AzEl[i].X);
			if (i > 0 && FMath::Abs(AzEl[i].X - AzEl[i - 1].X) > 180.0) Flush();
			else if (i > 0 && bVisible != bRunVisible) { Run.Add(P); Flush(); }
			bRunVisible = bVisible;
			Run.Add(P);
		}
		Flush();
	};
	Track(SunTrack, SunColor);
	Track(EarthTrack, EarthColor);

	// now: Sun and Earth at true angular size on the vertical scale, at least 8 px
	UGameInstance* GI = GetGameInstance();
	ULHEphemerisSubsystem* Eph = GI ? GI->GetSubsystem<ULHEphemerisSubsystem>() : nullptr;
	const ULHTimeSubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<ULHTimeSubsystem>() : nullptr;
	if (Eph && Time && !Site.Id.IsEmpty())
	{
		static const FSlateRoundedBoxBrush Dot(FLinearColor::White, 64.f);   // radius clamps to a circle
		const FLHSkyState Now = Eph->GetSkyState(Site, Time->GetTime());
		auto Marker = [&](double Az, double El, double RadiusDeg, const FLinearColor& Color)
		{
			const float D = FMath::Max(8.f, float(2.0 * RadiusDeg / (ElMax - ElMin)) * Sz.Y);
			FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(D, D), FSlateLayoutTransform(ToPx(Az, El) - FVector2f(D / 2.f))),
				&Dot, ESlateDrawEffect::None, Color);
		};
		Marker(Now.SunAzDeg, Now.SunElDeg, FMath::RadiansToDegrees(FMath::Asin(LHCore::SunRadiusKm / FMath::Max(Now.SunDistanceKm, 1.0))), SunColor);
		Marker(Now.EarthAzDeg, Now.EarthElDeg, FMath::RadiansToDegrees(FMath::Asin(LHCore::EarthRadiusKm / FMath::Max(Now.EarthDistanceKm, 1e5))), EarthColor);
	}
	return Layer + 3;
}
