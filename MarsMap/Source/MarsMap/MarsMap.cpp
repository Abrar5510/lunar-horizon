#include "MarsMap.h"

#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MarsMap, "MarsMap");
DEFINE_LOG_CATEGORY_STATIC(LogMars, Log, All);

namespace
{
// Planetocentric lat / east lon. Landers from mission reports; features and ice sites are approximate centres.
const FMarsSite GSites[] = {
	{TEXT("Perseverance"), TEXT("Lander"), 18.44, 77.45, TEXT("NASA rover, landed 2021 in Jezero crater's ancient river delta.")},
	{TEXT("Curiosity"), TEXT("Lander"), -4.59, 137.44, TEXT("NASA rover, landed 2012 in Gale crater; climbing Mount Sharp.")},
	{TEXT("InSight"), TEXT("Lander"), 4.50, 135.62, TEXT("NASA lander 2018-2022; measured marsquakes.")},
	{TEXT("Zhurong"), TEXT("Lander"), 25.07, 109.93, TEXT("CNSA rover, landed 2021 in Utopia Planitia.")},
	{TEXT("Opportunity"), TEXT("Lander"), -1.95, 354.47, TEXT("NASA rover 2004-2018; drove 45 km in Meridiani Planum.")},
	{TEXT("Spirit"), TEXT("Lander"), -14.57, 175.47, TEXT("NASA rover 2004-2010 in Gusev crater.")},
	{TEXT("Phoenix"), TEXT("Lander"), 68.22, 234.25, TEXT("NASA lander 2008; dug up water ice just under the soil.")},
	{TEXT("Mars Pathfinder"), TEXT("Lander"), 19.13, 326.78, TEXT("NASA lander + Sojourner rover, 1997, Ares Vallis.")},
	{TEXT("Viking 1"), TEXT("Lander"), 22.27, 312.05, TEXT("NASA's first successful Mars lander, 1976, Chryse Planitia.")},
	{TEXT("Viking 2"), TEXT("Lander"), 47.64, 134.29, TEXT("NASA lander, 1976, Utopia Planitia.")},
	{TEXT("Olympus Mons"), TEXT("Feature"), 18.65, 226.20, TEXT("Tallest volcano in the Solar System, ~22 km above its plains.")},
	{TEXT("Ascraeus Mons"), TEXT("Feature"), 11.92, 255.92, TEXT("Northernmost of the three Tharsis Montes volcanoes.")},
	{TEXT("Pavonis Mons"), TEXT("Feature"), 1.48, 247.04, TEXT("Central Tharsis volcano, on the equator.")},
	{TEXT("Arsia Mons"), TEXT("Feature"), -8.26, 239.91, TEXT("Southern Tharsis volcano; possible lava-tube skylights on its flanks.")},
	{TEXT("Elysium Mons"), TEXT("Feature"), 25.02, 147.21, TEXT("Second-largest volcanic province on Mars.")},
	{TEXT("Valles Marineris"), TEXT("Feature"), -13.9, 300.8, TEXT("Canyon system 4,000 km long and up to 7 km deep.")},
	{TEXT("Noctis Labyrinthus"), TEXT("Feature"), -7.0, 258.0, TEXT("Maze of fractured canyons at the head of Valles Marineris.")},
	{TEXT("Hellas Planitia"), TEXT("Feature"), -42.4, 70.5, TEXT("Giant impact basin; its floor has the thickest air on Mars.")},
	{TEXT("Argyre Planitia"), TEXT("Feature"), -49.7, 316.0, TEXT("Impact basin in the southern highlands.")},
	{TEXT("Utopia Planitia"), TEXT("Feature"), 49.7, 118.0, TEXT("Largest recognised impact basin; low, flat northern plains.")},
	{TEXT("Syrtis Major"), TEXT("Feature"), 8.4, 69.5, TEXT("Dark volcanic plain, the first surface feature seen from Earth.")},
	{TEXT("Gale Crater"), TEXT("Feature"), -5.4, 137.8, TEXT("154 km crater with a 5 km layered mound (Mount Sharp).")},
	{TEXT("Jezero Crater"), TEXT("Feature"), 18.38, 77.58, TEXT("Former lake with a preserved river delta.")},
	{TEXT("Planum Boreum"), TEXT("Ice"), 88.0, 15.0, TEXT("North polar cap: water ice up to 3 km thick.")},
	{TEXT("Planum Australe"), TEXT("Ice"), -83.9, 160.0, TEXT("South polar cap: water ice under a CO2 ice veneer.")},
	{TEXT("Arcadia Planitia"), TEXT("Ice"), 47.2, 184.3, TEXT("Candidate human landing region: water ice within ~1 m of the surface.")},
	{TEXT("Deuteronilus Mensae"), TEXT("Ice"), 43.9, 22.6, TEXT("Debris-covered glaciers of buried water ice.")},
	{TEXT("Phlegra Montes"), TEXT("Ice"), 40.4, 163.7, TEXT("Mid-latitude ice-rich terrain near the Arcadia/Utopia plains.")},
};

FLinearColor KindColor(const TCHAR* Kind)
{
	if (FCString::Strcmp(Kind, TEXT("Lander")) == 0) return FLinearColor(0.35f, 0.9f, 1.f);
	if (FCString::Strcmp(Kind, TEXT("Ice")) == 0) return FLinearColor(0.8f, 0.7f, 1.f);
	return FLinearColor(1.f, 0.85f, 0.35f);
}

FString Hm(double Hours) { return FString::Printf(TEXT("%02d:%02d"), int(Hours), int(FMath::Fmod(Hours * 60.0, 60.0))); }

constexpr double SolSeconds = 88775.244;
constexpr double RoverMetresPerSol = 200.0;   // rough multi-sol average for an autonomous-driving rover
} // namespace

TConstArrayView<FMarsSite> MarsSites() { return GSites; }

// ---------------------------------------------------------------- Globe

AMarsGlobe::AMarsGlobe()
{
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;
}

void AMarsGlobe::BeginPlay()
{
	Super::BeginPlay();
	const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Data"));
	Hi = MakeShared<MarsCore::FGrid>();
	Lo = MakeShared<MarsCore::FGrid>();
	auto Load = [&Dir](MarsCore::FGrid& Grid, const TCHAR* File, int Width) {
		if (Grid.Load(TCHAR_TO_UTF8(*(Dir / File)), Width)) return;
		UE_LOG(LogMars, Error, TEXT("Missing %s/%s: run tools/mars_prep.py. Using a flat planet."), *Dir, File);
		Grid.W = 4; Grid.H = 2; Grid.M.assign(8, 0);
	};
	Load(*Hi, TEXT("megt90n000eb.img"), 5760);
	Load(*Lo, TEXT("megt90n000cb.img"), 1440);
	TexMola = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Mars/T_MarsMola.T_MarsMola"));
	TexNatural = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Mars/T_MarsNatural.T_MarsNatural"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Mars/M_Globe.M_Globe"));
	if (!Base)
	{
		UE_LOG(LogMars, Error, TEXT("Missing /Game/Mars/M_Globe: run tools/mars_ue_setup.py."));
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	Mat = UMaterialInstanceDynamic::Create(Base, this);
	SetNatural(false);
	Rebuild();
}

void AMarsGlobe::SetNatural(bool bIn)
{
	bNatural = bIn;
	if (UTexture2D* T = bIn ? TexNatural : TexMola) Mat->SetTextureParameterValue(TEXT("Color"), T);
}

FVector AMarsGlobe::ToWorld(double Lat, double LonE, double ElevM) const
{
	double U[3];
	MarsCore::Unit(Lat, LonE, U);
	return FVector(U[0], U[1], U[2]) * ((MarsCore::RadiusM + ElevM * Exaggeration) * UUPerM);
}

void AMarsGlobe::Rebuild()
{
	constexpr int NLon = 720, NLat = 360, W = NLon + 1;   // 0.5 deg vertices; the seam column is duplicated for UVs
	TArray<FVector> V, N;
	TArray<FVector2D> UV;
	TArray<int32> T;
	V.SetNumUninitialized(W * (NLat + 1));
	N.SetNumUninitialized(V.Num());
	UV.SetNumUninitialized(V.Num());
	for (int J = 0; J <= NLat; ++J)
		for (int I = 0; I <= NLon; ++I)
		{
			const double Lat = 90.0 - J * 0.5, Lon = I * 0.5;
			V[J * W + I] = ToWorld(Lat, Lon, Hi->Sample(Lat, Lon));
			UV[J * W + I] = FVector2D(double(I) / NLon, double(J) / NLat);
		}
	for (int J = 0; J <= NLat; ++J)
		for (int I = 0; I <= NLon; ++I)
		{
			const FVector& P = V[J * W + I];
			if (J == 0 || J == NLat) { N[J * W + I] = P.GetSafeNormal(); continue; }
			const int Il = I == 0 ? NLon - 1 : I - 1, Ir = I == NLon ? 1 : I + 1;
			FVector Nn = FVector::CrossProduct(V[J * W + Ir] - V[J * W + Il], V[(J + 1) * W + I] - V[(J - 1) * W + I]).GetSafeNormal();
			N[J * W + I] = (Nn | P) < 0 ? -Nn : Nn;
		}
	T.Reserve(NLon * NLat * 6);
	for (int J = 0; J < NLat; ++J)
		for (int I = 0; I < NLon; ++I)
		{
			const int A = J * W + I, B = A + 1, C = A + W, D = C + 1;
			T.Append({A, C, B, B, C, D});   // front faces outward (two-sided shading flips normals on back faces)
		}
	Mesh->CreateMeshSection(0, V, T, N, UV, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	Mesh->SetMaterial(0, Mat);
}

bool AMarsGlobe::Pick(const FVector& O, const FVector& D, double& Lat, double& LonE) const
{
	double R = MarsCore::RadiusM * UUPerM;
	for (int K = 0; K < 4; ++K)   // hit the sphere, then re-hit at the local terrain radius
	{
		const double B = O | D, Disc = B * B - (O.SizeSquared() - R * R);
		if (Disc < 0) return K > 0;
		const FVector P = O + D * (-B - FMath::Sqrt(Disc));
		Lat = FMath::RadiansToDegrees(FMath::Asin(P.Z / P.Size()));
		LonE = MarsCore::Wrap(FMath::RadiansToDegrees(FMath::Atan2(-P.Y, P.X)), 360.0);
		R = (MarsCore::RadiusM + Hi->Sample(Lat, LonE) * Exaggeration) * UUPerM;
	}
	return true;
}

// ---------------------------------------------------------------- Player controller: camera, input, UI

AMarsPlayerController::AMarsPlayerController()
{
	bShowMouseCursor = true;
}

void AMarsPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	SimUnix = double(FDateTime::UtcNow().ToUnixTimestamp());
	UWorld* World = GetWorld();
	Globe = World->SpawnActor<AMarsGlobe>();
	SunLight = World->SpawnActor<ADirectionalLight>();
	SunLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SunLight->GetLightComponent()->SetIntensity(3.f);
	SunLight->GetLightComponent()->SetCastShadows(false);
	Cam = World->SpawnActor<ACameraActor>();
	Cam->GetCameraComponent()->SetFieldOfView(45.f);
	Cam->GetCameraComponent()->SetConstraintAspectRatio(false);
	SetViewTarget(Cam);

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	auto Font = [](int Size, const char* Style = "Regular") { return FCoreStyle::GetDefaultFontStyle(Style, Size); };
	auto Text = [&](TAttribute<FText> T, int Size = 10, const char* Style = "Regular") -> TSharedRef<SWidget> {
		return SNew(STextBlock).Text(T).Font(Font(Size, Style)).ColorAndOpacity(FLinearColor::White).AutoWrapText(true);
	};
	auto Live = [](TFunction<FString()> F) { return TAttribute<FText>::CreateLambda([F] { return FText::FromString(F()); }); };
	auto Button = [&](const TCHAR* Label, TFunction<void()> F) -> TSharedRef<SWidget> {
		return SNew(SButton).OnClicked_Lambda([F] { F(); return FReply::Handled(); })
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(Font(9))];
	};
	auto Panel = [](TSharedRef<SWidget> Content) -> TSharedRef<SWidget> {
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.8f))
			.Padding(12)
			.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })   // don't click through to the globe
			[SNew(SBox).WidthOverride(330)[Content]];
	};
	auto Gap = [] { return SNew(SBox).HeightOverride(10); };

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const FMarsSite& S : MarsSites())
	{
		List->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SButton).OnClicked_Lambda([this, &S] { FlyTo(S.Lat, S.LonE); Select(S.Lat, S.LonE); return FReply::Handled(); })
			[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s   (%s)"), S.Name, S.Kind))).Font(Font(9)).ColorAndOpacity(KindColor(S.Kind))]
		];
	}

	TSharedRef<SWidget> Left = Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("MARTIAN MAP")), 16, "Bold")]
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("Drag to pan, scroll to zoom, click to inspect.")), 9)]
		+ SVerticalBox::Slot().AutoHeight()[Gap()]
		+ SVerticalBox::Slot().AutoHeight()[Text(Live([this] { return TimeText(); }))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("Pause"), [this] { Speed = 0; })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("Real time"), [this] { Speed = 1; })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("1 h/s"), [this] { Speed = 3600; })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("1 sol/s"), [this] { Speed = SolSeconds; })]
			+ SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Now"), [this] { SimUnix = double(FDateTime::UtcNow().ToUnixTimestamp()); })]
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap()]
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("Places")), 11, "Bold")]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(380)[SNew(SScrollBox) + SScrollBox::Slot()[List]]]);

	TSharedRef<SWidget> Right = Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("Selected spot")), 11, "Bold")]
		+ SVerticalBox::Slot().AutoHeight()[Text(Live([this] { return SelectionText(); }))]
		+ SVerticalBox::Slot().AutoHeight()[Gap()]
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("Rover route")), 11, "Bold")]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("Set start (A)"), [this] { if (bHasSel) { bHasA = true; ALat = SelLat; ALon = SelLon; PlanRoute(); } })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)[Button(TEXT("Set end (B)"), [this] { if (bHasSel) { bHasB = true; BLat = SelLat; BLon = SelLon; PlanRoute(); } })]
			+ SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Clear"), [this] { bHasA = bHasB = false; Route = MarsCore::FRoute(); ++RouteGen; bRouting = false; })]
		]
		+ SVerticalBox::Slot().AutoHeight()[Text(Live([this] { return FString::Printf(TEXT("Max regional slope (15 km cells): %.1f°"), MaxSlopeDeg); }), 9)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SSlider)
			.Value_Lambda([this] { return (MaxSlopeDeg - 1.f) / 14.f; })
			.OnValueChanged_Lambda([this](float V) { MaxSlopeDeg = 1.f + V * 14.f; })
			.OnMouseCaptureEnd_Lambda([this] { PlanRoute(); })
		]
		+ SVerticalBox::Slot().AutoHeight()[Text(Live([this] { return RouteText(); }))]
		+ SVerticalBox::Slot().AutoHeight()[Gap()]
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("View")), 11, "Bold")]
		+ SVerticalBox::Slot().AutoHeight()[Text(Live([this] { return FString::Printf(TEXT("Vertical exaggeration: %.0fx"), Globe ? Globe->Exaggeration : 0.f); }), 9)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SSlider)
			.Value_Lambda([this] { return Globe ? (Globe->Exaggeration - 1.f) / 49.f : 0.f; })
			.OnValueChanged_Lambda([this](float V) { if (Globe) Globe->Exaggeration = FMath::RoundToFloat(1.f + V * 49.f); })
			.OnMouseCaptureEnd_Lambda([this] { if (Globe) Globe->Rebuild(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this] { return Globe && Globe->bNatural ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { if (Globe) Globe->SetNatural(S == ECheckBoxState::Checked); })
			[SNew(STextBlock).Text(FText::FromString(TEXT("Natural colours (off = MOLA elevation colours)"))).Font(Font(9))]
		]
		+ SVerticalBox::Slot().AutoHeight()[Text(FText::FromString(TEXT("Elevation: NASA MGS MOLA MEGDR. Sun and Mars time: Mars24 (NASA GISS), checked against JPL SPICE.")), 8)]);

	Ui = SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(16)[Left]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(16)[Right];
	if (UGameViewportClient* Vp = World->GetGameViewport()) Vp->AddViewportWidgetContent(Ui.ToSharedRef(), 10);
}

void AMarsPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	++RouteGen;   // drop any route still being planned
	if (Ui.IsValid())
		if (UGameViewportClient* Vp = GetWorld()->GetGameViewport()) Vp->RemoveViewportWidgetContent(Ui.ToSharedRef());
	Super::EndPlay(Reason);
}

void AMarsPlayerController::PlayerTick(float Dt)
{
	Super::PlayerTick(Dt);
	if (!Globe || !Cam) return;
	SimUnix += Dt * Speed;

	float Mx = 0, My = 0;
	const bool bMouse = GetMousePosition(Mx, My);
	const FVector2D M(Mx, My);
	if (bMouse && WasInputKeyJustPressed(EKeys::LeftMouseButton)) { PressPos = LastPos = M; bDragging = true; }
	if (bDragging && bMouse && IsInputKeyDown(EKeys::LeftMouseButton))
	{
		const FVector2D D = M - LastPos;
		LastPos = M;
		const double DegPerPx = 0.04 * AltKm / 3000.0;
		TgtLon = CamLon = MarsCore::Wrap(CamLon - D.X * DegPerPx / FMath::Max(0.2, FMath::Cos(FMath::DegreesToRadians(CamLat))), 360.0);
		TgtLat = CamLat = FMath::Clamp(CamLat + D.Y * DegPerPx, -85.0, 85.0);
	}
	if (bDragging && WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		bDragging = false;
		FVector O, Dir;
		double La, Lo;
		if (FVector2D::Distance(M, PressPos) < 4 && DeprojectMousePositionToWorld(O, Dir) && Globe->Pick(O, Dir, La, Lo)) Select(La, Lo);
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) TgtAltKm *= 0.8;
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) TgtAltKm *= 1.25;
	TgtAltKm = FMath::Clamp(TgtAltKm, 40.0, 30000.0);

	const double A = 1.0 - FMath::Exp(-Dt * 4.0);   // ease toward the fly-to / zoom target
	CamLat += (TgtLat - CamLat) * A;
	CamLon = MarsCore::Wrap(CamLon + FMath::UnwindDegrees(TgtLon - CamLon) * A, 360.0);
	AltKm *= FMath::Pow(TgtAltKm / AltKm, A);

	double U[3];
	MarsCore::Unit(CamLat, CamLon, U);
	const FVector Up(U[0], U[1], U[2]);
	Cam->SetActorLocationAndRotation(Up * ((MarsCore::RadiusM + AltKm * 1000.0) * AMarsGlobe::UUPerM),
	                                 FRotationMatrix::MakeFromXZ(-Up, FVector::UpVector).Rotator());
	if (GetViewTarget() != Cam) SetViewTarget(Cam);

	const MarsCore::FSun S = Sun();
	MarsCore::Unit(S.SubLat, S.SubLonE, U);
	SunLight->SetActorRotation(FVector(-U[0], -U[1], -U[2]).Rotation());   // light travels from the Sun toward Mars

	if (!PendingShot.IsEmpty() && !bRouting && FMath::Abs(AltKm / TgtAltKm - 1.0) < 0.01)
	{
		FScreenshotRequest::RequestScreenshot(PendingShot, true, false);
		UE_LOG(LogMars, Display, TEXT("Screenshot requested: %s"), *PendingShot);
		PendingShot.Empty();
	}
}

void AMarsPlayerController::MarsGo(float Lat, float LonE, float InAltKm)
{
	FlyTo(Lat, LonE, InAltKm);
	CamLat = TgtLat; CamLon = TgtLon; AltKm = TgtAltKm;   // jump, don't fly
	Select(Lat, LonE);
}

void AMarsPlayerController::MarsRoute(float LatA, float LonA, float LatB, float LonB)
{
	bHasA = bHasB = true;
	ALat = LatA; ALon = LonA; BLat = LatB; BLon = LonB;
	Select(LatB, LonB);
	PlanRoute();
}

void AMarsPlayerController::MarsNatural(bool bOn)
{
	if (Globe) Globe->SetNatural(bOn);
}

void AMarsPlayerController::MarsShot(const FString& File)
{
	PendingShot = FPaths::IsRelative(File) ? FPaths::ProjectSavedDir() / TEXT("Screenshots") / File : File;
}

void AMarsPlayerController::FlyTo(double Lat, double LonE, double InAltKm)
{
	TgtLat = FMath::Clamp(Lat, -85.0, 85.0);
	TgtLon = LonE;
	TgtAltKm = InAltKm;
}

void AMarsPlayerController::Select(double Lat, double LonE)
{
	bHasSel = true;
	SelLat = Lat;
	SelLon = LonE;
}

void AMarsPlayerController::PlanRoute()
{
	if (!bHasA || !bHasB || !Globe) return;
	bRouting = true;
	const int32 Gen = ++RouteGen;
	TWeakObjectPtr<AMarsPlayerController> Self(this);
	Async(EAsyncExecution::ThreadPool, [Self, Gen, G = Globe->Lo, La0 = ALat, Lo0 = ALon, La1 = BLat, Lo1 = BLon, Slope = MaxSlopeDeg]
	{
		MarsCore::FRoute R = MarsCore::PlanRoute(*G, La0, Lo0, La1, Lo1, Slope);
		AsyncTask(ENamedThreads::GameThread, [Self, Gen, R = MoveTemp(R)]() mutable
		{
			if (Self.IsValid() && Self->RouteGen == Gen) { Self->Route = MoveTemp(R); Self->bRouting = false; }
		});
	});
}

FString AMarsPlayerController::TimeText() const
{
	const MarsCore::FSun S = Sun();
	static const TCHAR* Seasons[] = {TEXT("northern spring"), TEXT("northern summer"), TEXT("northern autumn"), TEXT("northern winter")};
	return FString::Printf(TEXT("Earth: %s UTC\nMars Sol Date %.3f\nCoordinated Mars Time %s\nSeason: Ls %.1f°, %s\nSun distance %.3f AU"),
		*FDateTime::FromUnixTimestamp(int64(SimUnix)).ToString(TEXT("%Y-%m-%d %H:%M")), S.Msd, *Hm(S.MtcHours),
		S.LsDeg, Seasons[int(S.LsDeg / 90.0) % 4], S.DistAu);
}

FString AMarsPlayerController::SelectionText() const
{
	if (!bHasSel || !Globe) return TEXT("Click the globe or pick a place.");
	const MarsCore::FSun S = Sun();
	const double H = Globe->Hi->Sample(SelLat, SelLon);
	const double P = MarsCore::PressurePa(H);
	const double El = MarsCore::SunElevationDeg(S, SelLat, SelLon);
	const double Flux = El > 0 ? 1361.0 / (S.DistAu * S.DistAu) * FMath::Sin(FMath::DegreesToRadians(El)) : 0.0;
	const FMarsSite* Near = nullptr;
	double Best = 250e3;
	for (const FMarsSite& Site : MarsSites())
	{
		const double D = MarsCore::GreatCircleM(SelLat, SelLon, Site.Lat, Site.LonE);
		if (D < Best) { Best = D; Near = &Site; }
	}
	FString Out;
	if (Near) Out += FString::Printf(TEXT("%s (%.0f km away)\n%s\n\n"), Near->Name, Best / 1e3, Near->Note);
	Out += FString::Printf(TEXT("%.2f°%s, %.2f°E\n"), FMath::Abs(SelLat), SelLat >= 0 ? TEXT("N") : TEXT("S"), SelLon);
	Out += FString::Printf(TEXT("Elevation: %+.0f m\n"), H);
	Out += FString::Printf(TEXT("Air pressure: ~%.0f Pa (Earth: 101,325)\n"), P);
	Out += P > 611.657 ? TEXT("Above water's triple point: liquid water can briefly exist.\n")
	                   : TEXT("Below water's triple point: ice turns straight to vapour.\n");
	Out += FString::Printf(TEXT("Local solar time %s, Sun %+.0f° %s\n"), *Hm(MarsCore::LocalSolarHours(S, SelLon)), El, El > 0 ? TEXT("up") : TEXT("(night)"));
	Out += FString::Printf(TEXT("Sunlight at top of air: %.0f W/m²"), Flux);
	return Out;
}

FString AMarsPlayerController::RouteText() const
{
	if (!bHasA) return TEXT("Select a spot and press Set start.");
	if (!bHasB) return TEXT("Now select the destination and press Set end.");
	if (bRouting) return TEXT("Planning...");
	if (!Route.Ok) return FString::Printf(TEXT("No route stays under %.1f°. Raise the slope limit."), MaxSlopeDeg);
	return FString::Printf(TEXT("Drive %.0f km (straight line %.0f km)\nClimb +%.0f m, descend -%.0f m\nSteepest regional slope %.1f°\n~%.0f sols at %.0f m/sol"),
		Route.DistM / 1e3, MarsCore::GreatCircleM(ALat, ALon, BLat, BLon) / 1e3, Route.AscentM, Route.DescentM,
		Route.MaxSlopeDeg, Route.DistM / RoverMetresPerSol, RoverMetresPerSol);
}

// ---------------------------------------------------------------- HUD: labels, markers, route, profile

void AMarsHUD::DrawHUD()
{
	Super::DrawHUD();
	const AMarsPlayerController* PC = Cast<AMarsPlayerController>(PlayerOwner);
	if (!PC || !PC->Globe || !PC->PlayerCameraManager || !Canvas) return;
	const AMarsGlobe* G = PC->Globe;
	const FVector CamPos = PC->PlayerCameraManager->GetCameraLocation();
	UFont* Font = GEngine->GetSmallFont();
	const float Ts = FMath::Max(1.f, Canvas->ClipY / 1080.f);   // keep labels readable on Retina
	auto ToScreen = [&](const FVector& P, FVector2D& S) {
		if ((P.GetSafeNormal() | (CamPos - P).GetSafeNormal()) < 0.0) return false;   // behind the planet's limb
		const FVector V = Project(P);
		S = FVector2D(V.X, V.Y);
		return true;
	};

	TArray<FVector2D, TInlineAllocator<32>> Labels;
	for (const FMarsSite& S : MarsSites())
	{
		FVector2D P;
		if (!ToScreen(G->Surface(S.Lat, S.LonE), P)) continue;
		const FLinearColor C = KindColor(S.Kind);
		DrawRect(C, P.X - 3 * Ts, P.Y - 3 * Ts, 6 * Ts, 6 * Ts);
		// Sites are listed landers first, so a lander keeps its label when it sits on top of its crater.
		if (Labels.ContainsByPredicate([&](const FVector2D& L) { return FMath::Abs(L.Y - P.Y) < 14 * Ts && FMath::Abs(L.X - P.X) < 110 * Ts; })) continue;
		Labels.Add(P);
		DrawText(S.Name, C, P.X + 6 * Ts, P.Y - 7 * Ts, Font, Ts);
	}

	const MarsCore::FRoute& R = PC->Route;
	if (R.Ok && !PC->bRouting)
	{
		FVector2D Prev;
		bool bPrev = false;
		for (const MarsCore::FRoutePt& Pt : R.Pts)
		{
			FVector2D Cur;
			const bool bCur = ToScreen(G->Surface(Pt.Lat, Pt.LonE), Cur);
			if (bPrev && bCur) DrawLine(Prev.X, Prev.Y, Cur.X, Cur.Y, FLinearColor(1.f, 0.3f, 0.1f), 2.5f * Ts);
			Prev = Cur;
			bPrev = bCur;
		}
	}

	auto Marker = [&](double La, double Lo, const FLinearColor& C, const TCHAR* Label) {
		FVector2D P;
		if (!ToScreen(G->Surface(La, Lo), P)) return;
		const float K = 9 * Ts;
		DrawLine(P.X - K, P.Y, P.X + K, P.Y, C, 2 * Ts);
		DrawLine(P.X, P.Y - K, P.X, P.Y + K, C, 2 * Ts);
		if (Label) DrawText(Label, C, P.X + K, P.Y + 2, Font, 1.4f * Ts);
	};
	if (PC->bHasSel) Marker(PC->SelLat, PC->SelLon, FLinearColor::White, nullptr);
	if (PC->bHasA) Marker(PC->ALat, PC->ALon, FLinearColor(0.3f, 1.f, 0.4f), TEXT("A"));
	if (PC->bHasB) Marker(PC->BLat, PC->BLon, FLinearColor(1.f, 0.35f, 0.35f), TEXT("B"));

	if (R.Ok && !PC->bRouting && R.Pts.size() > 1)   // elevation profile along the route
	{
		const float W = 560 * Ts, H = 110 * Ts, X0 = (Canvas->ClipX - W) / 2, Y0 = Canvas->ClipY - H - 20 * Ts;
		double Lo = R.Pts[0].ElevM, Hi = Lo;
		for (const MarsCore::FRoutePt& Pt : R.Pts) { Lo = FMath::Min(Lo, Pt.ElevM); Hi = FMath::Max(Hi, Pt.ElevM); }
		const double Span = FMath::Max(Hi - Lo, 100.0);
		DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.8f), X0 - 12 * Ts, Y0 - 26 * Ts, W + 24 * Ts, H + 36 * Ts);
		DrawText(FString::Printf(TEXT("Elevation profile: %+.0f to %+.0f m over %.0f km"), Lo, Hi, R.DistM / 1e3), FLinearColor::White, X0, Y0 - 22 * Ts, Font, Ts);
		for (size_t I = 1; I < R.Pts.size(); ++I)
		{
			auto Xy = [&](const MarsCore::FRoutePt& Pt) { return FVector2D(X0 + W * Pt.DistM / R.DistM, Y0 + H * (1.0 - (Pt.ElevM - Lo) / Span)); };
			const FVector2D A = Xy(R.Pts[I - 1]), B = Xy(R.Pts[I]);
			DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(1.f, 0.55f, 0.2f), 2 * Ts);
		}
	}
}

// ---------------------------------------------------------------- Game mode

AMarsGameMode::AMarsGameMode()
{
	PlayerControllerClass = AMarsPlayerController::StaticClass();
	HUDClass = AMarsHUD::StaticClass();
	DefaultPawnClass = nullptr;
}
