#include "LHHorizonSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const FLHHorizonMask* ULHHorizonSubsystem::Find(const FString& Key)
{
	if (const FLHHorizonMask* Cached = Masks.Find(Key)) return Cached->ElevationDeg.Num() ? Cached : nullptr;

	FLHHorizonMask& M = Masks.Add(Key);   // a missing file caches as empty, so the disk is read once
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectContentDir() / TEXT("Data/Horizons") / (Key + TEXT(".csv"))));
	for (int32 i = 1; i < Lines.Num(); ++i)   // skip header
	{
		FString Az, El;
		if (Lines[i].Split(TEXT(","), &Az, &El)) M.ElevationDeg.Add(FCString::Atof(*El));
	}
	M.AzimuthStepDeg = 360.f / FMath::Max(1, M.ElevationDeg.Num());
	return M.ElevationDeg.Num() ? &M : nullptr;
}

bool ULHHorizonSubsystem::GetMask(const FString& Key, FLHHorizonMask& Out)
{
	const FLHHorizonMask* M = Find(Key);
	if (M) Out = *M;
	return M != nullptr;
}
