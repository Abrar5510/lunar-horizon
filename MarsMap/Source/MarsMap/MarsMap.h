#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "MarsCore.h"
#include "MarsMap.generated.h"

class ACameraActor;
class ADirectionalLight;
class SWidget;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UTexture2D;

struct FMarsSite
{
	const TCHAR* Name;
	const TCHAR* Kind;   // Lander, Feature, Ice
	double Lat, LonE;
	const TCHAR* Note;
};
TConstArrayView<FMarsSite> MarsSites();

// The planet: MOLA-displaced sphere at the world origin. 1 uu = 100 m.
UCLASS()
class AMarsGlobe : public AActor
{
	GENERATED_BODY()
public:
	static constexpr double UUPerM = 0.01;

	AMarsGlobe();
	virtual void BeginPlay() override;

	void Rebuild();                     // re-displace the mesh (after changing Exaggeration)
	void SetNatural(bool bNatural);
	FVector ToWorld(double Lat, double LonE, double ElevM) const;
	FVector Surface(double Lat, double LonE, double LiftM = 0) const { return ToWorld(Lat, LonE, Hi->Sample(Lat, LonE) + LiftM); }
	bool Pick(const FVector& Origin, const FVector& Dir, double& Lat, double& LonE) const;

	TSharedPtr<MarsCore::FGrid> Hi, Lo;   // 16 px/deg (mesh, readouts), 4 px/deg (routing)
	float Exaggeration = 10.f;
	bool bNatural = false;

private:
	UPROPERTY() UProceduralMeshComponent* Mesh;
	UPROPERTY() UMaterialInstanceDynamic* Mat;
	UPROPERTY() UTexture2D* TexMola;
	UPROPERTY() UTexture2D* TexNatural;
};

UCLASS()
class AMarsPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AMarsPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float Dt) override;

	void FlyTo(double Lat, double LonE, double AltKm = 1500);
	void Select(double Lat, double LonE);
	void PlanRoute();
	MarsCore::FSun Sun() const { return MarsCore::SunAt(SimUnix); }

	// Console commands for scripted demos and headless checks (-ExecCmds="MarsGo 18 77 3000, MarsShot gale.png")
	UFUNCTION(Exec) void MarsGo(float Lat, float LonE, float InAltKm);
	UFUNCTION(Exec) void MarsRoute(float LatA, float LonA, float LatB, float LonB);
	UFUNCTION(Exec) void MarsNatural(bool bOn);
	UFUNCTION(Exec) void MarsShot(const FString& File);   // waits for the camera and route to settle
	FString SelectionText() const;
	FString RouteText() const;
	FString TimeText() const;

	UPROPERTY() AMarsGlobe* Globe;
	UPROPERTY() ADirectionalLight* SunLight;
	UPROPERTY() ACameraActor* Cam;

	double CamLat = 12, CamLon = 290, AltKm = 9000;          // camera looks straight down at (CamLat, CamLon)
	double TgtLat = 12, TgtLon = 290, TgtAltKm = 9000;
	double SimUnix = 0, Speed = 1;
	bool bHasSel = false, bHasA = false, bHasB = false, bRouting = false;
	double SelLat = 0, SelLon = 0, ALat = 0, ALon = 0, BLat = 0, BLon = 0;
	float MaxSlopeDeg = 5.f;
	MarsCore::FRoute Route;

private:
	TSharedPtr<SWidget> Ui;
	FVector2D PressPos, LastPos;
	bool bDragging = false;
	int32 RouteGen = 0;
	FString PendingShot;
};

UCLASS()
class AMarsHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};

UCLASS()
class AMarsGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AMarsGameMode();
};
