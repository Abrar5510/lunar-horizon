#include "LHAnalysisSubsystem.h"
#include "LHEphemerisSubsystem.h"
#include "LHHorizonSubsystem.h"
#include "LHSpice.h"
#include "Async/Async.h"

void ULHAnalysisSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<ULHEphemerisSubsystem>();   // loads the kernels
}

void ULHAnalysisSubsystem::RunSweepAsync(const TArray<FLHSite>& Sites, FDateTime StartUtc, float Days, float StepMinutes)
{
	// Resolve masks here: the horizon cache is game-thread only
	ULHHorizonSubsystem* Horizons = GetGameInstance()->GetSubsystem<ULHHorizonSubsystem>();
	TArray<TArray<float>> Masks;
	for (const FLHSite& Site : Sites)
	{
		const FLHHorizonMask* M = Horizons->Find(Site.Id);
		Masks.Add(M ? M->ElevationDeg : TArray<float>());
	}
	const double Et0 = ULHEphemerisSubsystem::UtcToEt(StartUtc);
	const int32 N = int32(double(Days) * 1440.0 / StepMinutes);

	Async(EAsyncExecution::Thread, [WeakThis = TWeakObjectPtr<ULHAnalysisSubsystem>(this), Sites, SiteMasks = MoveTemp(Masks),
		Et0, N, StartUtc, StepMinutes]()
	{
		TArray<FLHSweepResult> Results;
		for (int32 s = 0; s < Sites.Num(); ++s)
		{
			FLHSweepResult& R = Results.AddDefaulted_GetRef();
			R.Site = Sites[s];
			R.StartUtc = StartUtc;
			R.StepMinutes = StepMinutes;
			double Obs[3];
			LHCore::SiteVectorKm(R.Site.LatDeg, R.Site.LonDeg, R.Site.HeightM, Obs);
			LHCore::FMetricsAccum Acc;
			Acc.StepMin = StepMinutes;
			for (int32 i = 0; i < N; ++i)
			{
				const double Et = Et0 + i * StepMinutes * 60.0;
				FLHSky Sky;
				if (!FLHSpice::Sky(Et, Obs, SiteMasks[s].GetData(), SiteMasks[s].Num(), Sky)) break;
				const bool bDsn = FLHSpice::AnyDsnSeesMoon(Et);
				Acc.Add(Sky.SunFrac, Sky.EarthVis, bDsn, LHCore::PanelPowerW(Sky.Sun.RangeKm, Sky.SunFrac, Sky.Sun.ElDeg));
				R.SunFraction.Add(float(Sky.SunFrac));
				R.EarthVisible.Add(Sky.EarthVis);
				R.DSN.Add(bDsn);
				R.SunAz.Add(float(Sky.Sun.AzDeg));
				R.SunEl.Add(float(Sky.Sun.ElDeg));
				R.EarthAz.Add(float(Sky.Earth.AzDeg));
				R.EarthEl.Add(float(Sky.Earth.ElDeg));
			}
			FLHSiteMetrics& M = R.Metrics;
			M.IlluminationPct = float(Acc.IlluminationPct());
			M.LongestDarkHours = float(Acc.LongestDarkHours());
			M.DTEPct = float(Acc.DTEPct());
			M.DSNCoveragePct = float(Acc.DSNCoveragePct());
			M.AvgPowerW = float(Acc.AvgPowerW());
			M.bHasNextSunrise = Acc.NextSunrise >= 0;
			M.NextSunriseUtc = StartUtc + FTimespan::FromMinutes(FMath::Max(0, Acc.NextSunrise) * StepMinutes);
			M.bHasNextEarthrise = Acc.NextEarthrise >= 0;
			M.NextEarthriseUtc = StartUtc + FTimespan::FromMinutes(FMath::Max(0, Acc.NextEarthrise) * StepMinutes);
		}
		AsyncTask(ENamedThreads::GameThread, [WeakThis, Done = MoveTemp(Results)]()
		{
			if (ULHAnalysisSubsystem* This = WeakThis.Get()) This->OnSweepDone.Broadcast(Done);
		});
	});
}
