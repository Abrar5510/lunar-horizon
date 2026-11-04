#include "UI/LHTimelineWidget.h"
#include "LHTimeSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

int32 ULHTimelineWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, G, Cull, Out, Layer, Style, bParentEnabled);
	const FVector2f Sz(G.GetLocalSize());
	const int32 N = Sweep.SunFraction.Num();
	const float PlotW = Sz.X - LabelW, RowH = Sz.Y / 4.f;
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);

	struct FRow { const TCHAR* Label; FLinearColor Color; TFunction<bool(int32)> On; };
	const FRow Rows[] = {
		{TEXT("Sunlit"), FLinearColor(1.0f, 0.66f, 0.10f), [this](int32 i) { return Sweep.SunFraction[i] >= 0.5f; }},
		{TEXT("Earth"), FLinearColor(0.25f, 0.61f, 1.0f), [this](int32 i) { return Sweep.EarthVisible[i]; }},
		{TEXT("DSN"), FLinearColor(0.78f, 0.80f, 0.82f), [this](int32 i) { return Sweep.DSN[i]; }},
		{TEXT("DTE"), FLinearColor(0.25f, 0.61f, 1.0f), [this](int32 i) { return Sweep.EarthVisible[i] && Sweep.DSN[i]; }},
	};
	for (int32 r = 0; r < 4; ++r)
	{
		const float Y = r * RowH;
		FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(LabelW, RowH), FSlateLayoutTransform(FVector2f(4.f, Y + RowH / 2.f - 8.f))),
			FString(Rows[r].Label), Font, ESlateDrawEffect::None, FLinearColor(0.78f, 0.80f, 0.82f));
		// one box per run of "on" steps
		for (int32 i = 0; i < N;)
		{
			if (!Rows[r].On(i)) { ++i; continue; }
			const int32 Start = i;
			while (i < N && Rows[r].On(i)) ++i;
			FSlateDrawElement::MakeBox(Out, Layer,
				G.ToPaintGeometry(FVector2f(float(i - Start) / N * PlotW, RowH * 0.6f), FSlateLayoutTransform(FVector2f(LabelW + float(Start) / N * PlotW, Y + RowH * 0.2f))),
				White, ESlateDrawEffect::None, Rows[r].Color);
		}
	}

	const ULHTimeSubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<ULHTimeSubsystem>() : nullptr;
	if (Time && N > 0)
	{
		const double F = (Time->GetTime() - Sweep.StartUtc).GetTotalSeconds() / Window().GetTotalSeconds();
		if (F >= 0.0 && F <= 1.0)
		{
			const float X = LabelW + float(F) * PlotW;
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{FVector2f(X, 0.f), FVector2f(X, Sz.Y)},
				ESlateDrawEffect::None, FLinearColor::White, true, 1.5f);
		}
	}
	return Layer + 2;
}

void ULHTimelineWidget::ScrubTo(const FGeometry& G, const FPointerEvent& E) const
{
	ULHTimeSubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<ULHTimeSubsystem>() : nullptr;
	if (!Time || Sweep.SunFraction.Num() == 0) return;
	const double X = G.AbsoluteToLocal(E.GetScreenSpacePosition()).X;
	const double F = FMath::Clamp((X - LabelW) / (double(G.GetLocalSize().X) - LabelW), 0.0, 1.0);
	Time->SetTime(Sweep.StartUtc + Window() * F);
}

FReply ULHTimelineWidget::NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnMouseButtonDown(G, E);
	ScrubTo(G, E);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply ULHTimelineWidget::NativeOnMouseMove(const FGeometry& G, const FPointerEvent& E)
{
	if (!HasMouseCapture()) return Super::NativeOnMouseMove(G, E);
	ScrubTo(G, E);
	return FReply::Handled();
}

FReply ULHTimelineWidget::NativeOnMouseButtonUp(const FGeometry& G, const FPointerEvent& E)
{
	return HasMouseCapture() ? FReply::Handled().ReleaseMouseCapture() : Super::NativeOnMouseButtonUp(G, E);
}
