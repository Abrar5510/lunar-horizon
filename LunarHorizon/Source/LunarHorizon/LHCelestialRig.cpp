#include "LHCelestialRig.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumGeoreference.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "LHEphemerisSubsystem.h"
#include "LHTimeSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/ConstructorHelpers.h"

ALHCelestialRig::ALHCelestialRig()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// docs/05 section 3: real solar disk, white light (no air to redden it), no atmosphere
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetLightSourceAngle(0.533f);
	Sun->SetUseTemperature(true);
	Sun->SetTemperature(5778.f);
	Sun->SetAtmosphereSunLight(false);

	Earthshine = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Earthshine"));
	Earthshine->SetupAttachment(RootComponent);
	Earthshine->SetMobility(EComponentMobility::Movable);
	Earthshine->SetLightSourceAngle(1.9f);   // Earth's disk: very soft shadows
	Earthshine->SetUseTemperature(true);
	Earthshine->SetTemperature(7000.f);
	Earthshine->SetAtmosphereSunLight(false);
	Earthshine->SetCastShadows(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	EarthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Earth"));
	StarSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stars"));
	for (UStaticMeshComponent* C : {EarthMesh.Get(), StarSphere.Get()})
	{
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(SphereMesh.Object);
		C->SetCastShadow(false);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	StarSphere->SetRelativeScale3D(FVector(2.0e7));   // 10,000 km radius around the camera; M_Stars is unlit, two-sided
}

FVector ALHCelestialRig::ToUnreal(const FVector& V) const
{
	if (Georef)
	{
		// Cesium returns the georeference's parent frame; apply the actor transform for world space
		return Georef->GetActorTransform().TransformVectorNoScale(
			Georef->TransformEarthCenteredEarthFixedDirectionToUnreal(V)).GetSafeNormal();
	}
	const double La = FMath::DegreesToRadians(Site.LatDeg), Lo = FMath::DegreesToRadians(Site.LonDeg);
	const FVector East(-FMath::Sin(Lo), FMath::Cos(Lo), 0.0);
	const FVector North(-FMath::Sin(La) * FMath::Cos(Lo), -FMath::Sin(La) * FMath::Sin(Lo), FMath::Cos(La));
	const FVector Up(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
	return FVector(V | East, -(V | North), V | Up);   // +X east, +Y south, +Z up
}

void ALHCelestialRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UGameInstance* GI = GetGameInstance();
	ULHEphemerisSubsystem* Eph = GI ? GI->GetSubsystem<ULHEphemerisSubsystem>() : nullptr;
	const ULHTimeSubsystem* Time = GetWorld()->GetSubsystem<ULHTimeSubsystem>();
	if (!Eph || !Eph->IsReady() || !Time) return;

	const FDateTime Utc = Time->GetTime();
	Sky = Eph->GetSkyState(Site, Utc);
	const FVector SunDir = ToUnreal(Sky.SunDirMoonMe), EarthDir = ToUnreal(Sky.EarthDirMoonMe);

	// A directional light shines along its +X: point it from the body toward the scene
	Sun->SetWorldRotation((-SunDir).Rotation());
	Sun->SetIntensity(float(LHCore::SunLux * Sky.SunVisibleFraction * FMath::Square(LHCore::AuKm / Sky.SunDistanceKm)));
	Earthshine->SetWorldRotation((-EarthDir).Rotation());
	Earthshine->SetIntensity(Sky.EarthshineLux);

	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// Earth disk at its true angular size, 500 km out (beyond the hero terrain). Below the
		// geometric horizon the Moon itself hides it.
		const double D = 5.0e7, RadiusDeg = FMath::RadiansToDegrees(FMath::Asin(LHCore::EarthRadiusKm / Sky.EarthDistanceKm));
		EarthMesh->SetWorldLocation(Cam->GetCameraLocation() + EarthDir * D);
		EarthMesh->SetWorldScale3D(FVector(D * FMath::Tan(FMath::DegreesToRadians(RadiusDeg)) / 50.0));   // engine sphere radius is 50 cm
		EarthMesh->SetVisibility(Sky.EarthElDeg + RadiusDeg > 0.0);
		StarSphere->SetWorldLocation(Cam->GetCameraLocation());
	}

	double R[3][3];   // columns are the J2000 axes in MOON_ME
	if (Eph->J2000ToMoonMe(Utc, R))
	{
		const FVector X = ToUnreal(FVector(R[0][0], R[1][0], R[2][0]));
		const FVector Y = -ToUnreal(FVector(R[0][1], R[1][1], R[2][1]));
		const FVector Z = ToUnreal(FVector(R[0][2], R[1][2], R[2][2]));
		StarSphere->SetWorldRotation(FMatrix(X, Y, Z, FVector::ZeroVector).Rotator());
	}

	if (MPC)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, MPC, TEXT("SunDirWS"), FLinearColor(SunDir));
		UKismetMaterialLibrary::SetVectorParameterValue(this, MPC, TEXT("EarthDirWS"), FLinearColor(EarthDir));
		UKismetMaterialLibrary::SetScalarParameterValue(this, MPC, TEXT("SunVisible"), Sky.SunVisibleFraction);
		UKismetMaterialLibrary::SetScalarParameterValue(this, MPC, TEXT("EarthPhase"), Sky.EarthPhase);
	}
}
