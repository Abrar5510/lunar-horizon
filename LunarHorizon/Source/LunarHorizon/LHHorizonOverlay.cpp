#include "LHHorizonOverlay.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

static FVector AzElToUnreal(double AzDeg, double ElDeg)   // +X east, +Y south, +Z up
{
	const double A = FMath::DegreesToRadians(AzDeg), E = FMath::DegreesToRadians(ElDeg);
	return FVector(FMath::Sin(A) * FMath::Cos(E), -FMath::Cos(A) * FMath::Cos(E), FMath::Sin(E));
}

ALHHorizonOverlay::ALHHorizonOverlay()
{
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCastShadow(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALHHorizonOverlay::SetMask(const FLHHorizonMask& Mask)
{
	TArray<FVector> Dirs;
	for (int32 i = 0; Mask.ElevationDeg.Num() && i <= Mask.ElevationDeg.Num(); ++i)   // repeat the first bin to close the ring
	{
		Dirs.Add(AzElToUnreal(i * Mask.AzimuthStepDeg, Mask.ElevationDeg[i % Mask.ElevationDeg.Num()]));
	}
	BuildRibbon(0, Dirs, FLinearColor(0.85f, 0.88f, 0.92f));
}

void ALHHorizonOverlay::SetTracks(const FLHSweepResult& Sweep)
{
	TArray<FVector> Sun, Earth;
	for (int32 i = 0; i < Sweep.SunAz.Num(); ++i)
	{
		Sun.Add(AzElToUnreal(Sweep.SunAz[i], Sweep.SunEl[i]));
		Earth.Add(AzElToUnreal(Sweep.EarthAz[i], Sweep.EarthEl[i]));
	}
	BuildRibbon(1, Sun, FLinearColor(1.0f, 0.66f, 0.10f));    // #FFA81A
	BuildRibbon(2, Earth, FLinearColor(0.25f, 0.61f, 1.0f));  // #3F9BFF
}

void ALHHorizonOverlay::BuildRibbon(int32 Section, const TArray<FVector>& Dirs, const FLinearColor& Color)
{
	if (Dirs.Num() < 2) { Mesh->ClearMeshSection(Section); return; }
	TArray<FVector> V, N;
	TArray<int32> T;
	TArray<FVector2D> UV;
	TArray<FLinearColor> C;
	const FVector HalfUp = FVector::UpVector * WidthCm * 0.5f;   // vertical ribbon reads well at grazing angles
	for (int32 i = 0; i < Dirs.Num(); ++i)
	{
		const FVector P = Dirs[i] * RadiusCm;
		const float U = float(i) / (Dirs.Num() - 1);
		V.Append({P - HalfUp, P + HalfUp});
		N.Append({-Dirs[i], -Dirs[i]});
		UV.Append({FVector2D(U, 0), FVector2D(U, 1)});
		C.Append({Color, Color});
		if (i > 0)
		{
			const int32 B = 2 * i;
			T.Append({B - 2, B - 1, B, B - 1, B + 1, B});
		}
	}
	Mesh->CreateMeshSection_LinearColor(Section, V, T, N, UV, C, {}, /*bCreateCollision*/ false);
	Mesh->SetMaterial(Section, TrackMaterial);
}
