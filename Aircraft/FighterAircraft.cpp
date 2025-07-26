// @2023 All rights reserved by Reverse-Alpha Studios


#include "FighterAircraft.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/RocketMovementComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

#include "Projectile.h"
#include "ProjectileRocket.h"
#include "TimerManager.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AFighterAircraft::AFighterAircraft()
{
	FVector FighterAircraftBoxExtent(600.0f, 425.0f, 100.0f);
	AreaCollision->SetBoxExtent(FighterAircraftBoxExtent);

	TargettingAerialStrikeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TargettingCamera"));
	TargettingAerialStrikeCamera->SetupAttachment(AircraftsMesh);
	TargettingAerialStrikeCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

void AFighterAircraft::BeginPlay()
{
	Super::BeginPlay();
	TargettingAerialStrikeCamera->SetActive(false);
}

void AFighterAircraft::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UStaticMeshSocket* RocketRightSocket = AircraftsMesh->GetSocketByName(FName("RocketRight"));

	UWorld* World = GetWorld();
	if (RocketRightSocket && World)
	{
		FTransform RightSocketTransform;
		bool bRightSuccess = RocketRightSocket->GetSocketTransform(RightSocketTransform, AircraftsMesh);
		if (bRightSuccess)
		{
			FVector SocketLocation  = RightSocketTransform.GetLocation();
			FVector ForwardVector	= RightSocketTransform.GetRotation().GetForwardVector(); // Get the forward vector of the socket
			FVector EndPoint		= SocketLocation + ForwardVector * 100.0f; // Extend the line by 100 units in the forward direction
		}
	}

	ResetDataTimer += GetWorld()->GetDeltaSeconds();
	float ResetDataDelay = 1.5f;


	/*monitoring values*/
	//if (ResetDataTimer >= ResetDataDelay)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("CurrentSpeed: %f"), CurrentSpeed);
	//	UE_LOG(LogTemp, Warning, TEXT("CurrentPitch: %f"), CurrentPitch);
	//	UE_LOG(LogTemp, Warning, TEXT("CurrentYaw: %f"),   CurrentYaw);
	//	UE_LOG(LogTemp, Warning, TEXT("CurrentRoll: %f"),  CurrentRoll);
	//	UE_LOG(LogTemp, Warning, TEXT("BoosterSpeed: %f"), BoostSpeed);

	//	ResetDataTimer = 0.0f;
	//}

	CheckAndFixTargettingCameraModeIfSwitched();
}

void AFighterAircraft::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	AAircraft::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(FireRocketInputAction,			ETriggerEvent::Triggered,	this, &AFighterAircraft::LaunchRocket);
		EnhancedInputComponent->BindAction(FireProjectileInputAction,		ETriggerEvent::Triggered,	this, &AFighterAircraft::FireTurret);
		EnhancedInputComponent->BindAction(SwitchTurretModeInputAction,		ETriggerEvent::Started,		this, &AFighterAircraft::InputAction_SwitchTurretMode);
		EnhancedInputComponent->BindAction(SwitchExplosiveModeInputAction,	ETriggerEvent::Started,		this, &AFighterAircraft::InputAction_SwitchExplosiveMode);
		EnhancedInputComponent->BindAction(FireProjectileInputAction,		ETriggerEvent::Completed,	this, &AFighterAircraft::SingleFireTurretEnd);
	}
}

#pragma region Inputs
void AFighterAircraft::InputAction_SwitchTurretMode()
{
	bMultiTurret = !bMultiTurret;
}

void AFighterAircraft::InputAction_SwitchExplosiveMode()
{
	bRocketMode = !bRocketMode;

	if (bRocketMode)
	{
		if (Cache_InteriorCamera)
		{
			if (InteriorCamera->IsActive() == false)
				InteriorCamera->SetActive(true);
		}
		else
		{
			if (BehindCamera->IsActive() == false)
				BehindCamera->SetActive(true);
		}
		if (TargettingAerialStrikeCamera->IsActive())
			TargettingAerialStrikeCamera->SetActive(false);
	}
	else
	{
		if (BehindCamera->IsActive())
			BehindCamera->SetActive(false);

		if (InteriorCamera->IsActive())
			InteriorCamera->SetActive(false);

		if (FrontCamera->IsActive())
			FrontCamera->SetActive(false);

		if(TargettingAerialStrikeCamera->IsActive() == false) 
			TargettingAerialStrikeCamera->SetActive(true);
	}
}

/*Camera Functions*/
void AFighterAircraft::CheckAndFixTargettingCameraModeIfSwitched()
{
	if (bCameraSwitchedWhileTargettingCameraOn)
	{
		bRocketMode = !bRocketMode;
		bCameraSwitchedWhileTargettingCameraOn = false;
	}
}
#pragma endregion

#pragma region Fire-Systems



void AFighterAircraft::FireTurret()
{
	/*This function, FireTurret(), is responsible for firing turrets on a fighter Aircraft object. It first checks if the turret can fire and if an aerial strike camera is not active. Depending on whether the Aircraft has multiple turrets or not, it calculates the firing direction and spawns projectiles accordingly, accompanied by appropriate sound effects. After firing, it sets a delay before the turret can fire again and logs various checkpoints for debugging purposes. */
	if (bCanFireTurret == false || TargettingAerialStrikeCamera->IsActive()) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();

	if (bMultiTurret)
	{
		if (TurretFireDelay != 0.10f)
		{
			TurretFireDelay = 0.10f;
		}

		const UStaticMeshSocket* TurretRightSocket = AircraftsMesh->GetSocketByName(FName("TurretRight"));
		if (World && TurretRightSocket)
		{
			FTransform RightTurretTransform;
			bool bRightSuccess = TurretRightSocket->GetSocketTransform(RightTurretTransform, AircraftsMesh);
			if (bRightSuccess)
			{
				FVector SocketLocation = RightTurretTransform.GetLocation();
				FVector ForwardVector = RightTurretTransform.GetRotation().GetForwardVector();
				FVector EndPoint = SocketLocation + ForwardVector * 100.0f;

				FActorSpawnParameters	SpawnParameters;
				SpawnParameters.Owner = GetOwner();
				SpawnParameters.Instigator = InstigatorPawn;

				AProjectile* RightProjectile = nullptr;
				RightProjectile = World->SpawnActor<AProjectile>
					(
						ProjectileClass,
						EndPoint,
						RightTurretTransform.Rotator(),
						SpawnParameters
					);
			}

			if (TurretFireSound != nullptr)
			{
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(TurretFireSound, AircraftsMesh, FName("TurretRight"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(RightTurretTransform.GetLocation(), RightTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, RightTurretTransform.GetLocation());
			}
		}

		const UStaticMeshSocket* TurretLeftSocket = AircraftsMesh->GetSocketByName(FName("TurretLeft"));
		if (World && TurretLeftSocket)
		{
			FTransform LeftTurretTransform;
			bool bRightSuccess = TurretLeftSocket->GetSocketTransform(LeftTurretTransform, AircraftsMesh);
			if (bRightSuccess)
			{
				FVector SocketLocation = LeftTurretTransform.GetLocation();
				FVector ForwardVector = LeftTurretTransform.GetRotation().GetForwardVector();
				FVector EndPoint = SocketLocation + ForwardVector * 100.0f;

				FActorSpawnParameters	SpawnParameters;
				SpawnParameters.Owner = GetOwner();
				SpawnParameters.Instigator = InstigatorPawn;

				AProjectile* LeftProjectile = nullptr;
				LeftProjectile = World->SpawnActor<AProjectile>
					(
						ProjectileClass,
						EndPoint,
						LeftTurretTransform.Rotator(),
						SpawnParameters
					);
			}
			if (TurretFireSound != nullptr)
			{
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(TurretFireSound, AircraftsMesh, FName("TurretLeft"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(LeftTurretTransform.GetLocation(), LeftTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, LeftTurretTransform.GetLocation());
			}
		}
	}
	else
	{
		if (TurretFireDelay != 0.05f)
		{
			TurretFireDelay = 0.05f;
		}
		const UStaticMeshSocket* TurretMiddleSocket = AircraftsMesh->GetSocketByName(FName("TurretMiddle"));
		if (World && TurretMiddleSocket)
		{
			FTransform MiddleTurretTransform;
			bool bMiddleSuccess = TurretMiddleSocket->GetSocketTransform(MiddleTurretTransform, AircraftsMesh);
			if (bMiddleSuccess)
			{
				FVector SocketLocation = MiddleTurretTransform.GetLocation();
				FVector ForwardVector = MiddleTurretTransform.GetRotation().GetForwardVector();
				FVector EndPoint = SocketLocation + ForwardVector * 100.0f;

				FActorSpawnParameters	SpawnParameters;
				SpawnParameters.Owner = GetOwner();
				SpawnParameters.Instigator = InstigatorPawn;

				AProjectile* LeftProjectile = nullptr;
				LeftProjectile = World->SpawnActor<AProjectile>
					(
						ProjectileClass,
						EndPoint,
						MiddleTurretTransform.Rotator(),
						SpawnParameters
					);
			}
			if (SingleTurretFireSoundStart != nullptr)
			{
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(SingleTurretFireSoundStart, AircraftsMesh, FName("TurretMiddle"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(MiddleTurretTransform.GetLocation(), MiddleTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, LeftTurretTransform.GetLocation());
			}
		}
	}
	bCanFireTurret = false;

	GetWorldTimerManager().SetTimer
	(
		TurretTimer,
		this,
		&AFighterAircraft::TurretTimerFinished,
		TurretFireDelay
	);
}

void AFighterAircraft::SingleFireTurretEnd()
{
	if (bMultiTurret == true || TargettingAerialStrikeCamera->IsActive()) return;
	if (SingleTurretFireSoundEnd != nullptr)
	{
		const UStaticMeshSocket* TurretMiddleSocket = AircraftsMesh->GetSocketByName(FName("TurretMiddle"));
		if (TurretMiddleSocket)
		{
			FTransform MiddleTurretTransform;
			bool bMiddleSuccess = TurretMiddleSocket->GetSocketTransform(MiddleTurretTransform, AircraftsMesh);
			if (bMiddleSuccess)
			{
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(SingleTurretFireSoundEnd, AircraftsMesh, FName("TurretMiddle"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(MiddleTurretTransform.GetLocation(), MiddleTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, LeftTurretTransform.GetLocation());
			}
		}
	}
}

void AFighterAircraft::TurretTimerFinished()
{
	bCanFireTurret = true;
}

void AFighterAircraft::LaunchRocket()
{
	if (bRocketMode)
	{
		if (bCanFireRocket == false) return;

		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		UWorld* World = GetWorld();
		const UStaticMeshSocket* RocketRightSocket = AircraftsMesh->GetSocketByName(FName("RocketRight"));
		if (World && RocketRightSocket)
		{
			FTransform RightSocketTransform;
			bool bRightSuccess = RocketRightSocket->GetSocketTransform(RightSocketTransform, AircraftsMesh);
			if (bRightSuccess)
			{
				FVector SocketLocation = RightSocketTransform.GetLocation();
				FVector ForwardVector = RightSocketTransform.GetRotation().GetForwardVector();
				FVector EndPoint = SocketLocation + ForwardVector * 100.0f;

				FActorSpawnParameters	SpawnParameters;
				SpawnParameters.Owner = GetOwner();
				SpawnParameters.Instigator = InstigatorPawn;

				AProjectileRocket* RightProjectileRocket = nullptr;
				RightProjectileRocket = World->SpawnActor<AProjectileRocket>
					(
						ProjectileRocketClass,
						EndPoint,
						RightSocketTransform.Rotator(),
						SpawnParameters
					);
			}
		}

		const UStaticMeshSocket* RocketLeftSocket = AircraftsMesh->GetSocketByName(FName("RocketLeft"));
		if (World && RocketLeftSocket)
		{
			FTransform LeftSocketTransform;
			bool bLeftSuccess = RocketLeftSocket->GetSocketTransform(LeftSocketTransform, AircraftsMesh);
			if (bLeftSuccess)
			{
				FVector SocketLocation = LeftSocketTransform.GetLocation();
				FVector ForwardVector = LeftSocketTransform.GetRotation().GetForwardVector();
				FVector EndPoint = SocketLocation + ForwardVector * 100.0f;

				FActorSpawnParameters	SpawnParameters;
				SpawnParameters.Owner = GetOwner();
				SpawnParameters.Instigator = InstigatorPawn;

				AProjectileRocket* LeftProjectileRocket = nullptr;
				LeftProjectileRocket = World->SpawnActor<AProjectileRocket>
					(
						ProjectileRocketClass,
						EndPoint,
						LeftSocketTransform.Rotator(),
						SpawnParameters
					);
			}
		}

		if (RocketAmmoEjectClass)
		{
			const UStaticMeshSocket* RightRocketAmmoEjectSocket = AircraftsMesh->GetSocketByName(FName("RocketAmmoEjectRight"));
			if (RightRocketAmmoEjectSocket != nullptr)
			{
				FTransform RightRocketEjectSocketTransform;
				bool bRightSuccess = RightRocketAmmoEjectSocket->GetSocketTransform(RightRocketEjectSocketTransform, AircraftsMesh);
				if (bRightSuccess)
				{
					if (World)
					{
						World->SpawnActor<AAmmoEject>
							(
								RocketAmmoEjectClass,
								RightRocketEjectSocketTransform.GetLocation(),
								RightRocketEjectSocketTransform.GetRotation().Rotator()
							);
					}
				}
			}

			const UStaticMeshSocket* LeftRocketAmmoEjectSocket = AircraftsMesh->GetSocketByName(FName("RocketAmmoEjectLeft"));
			if (LeftRocketAmmoEjectSocket != nullptr)
			{
				FTransform LeftRocketEjectSocketTransform;
				bool bLeftSuccess = LeftRocketAmmoEjectSocket->GetSocketTransform(LeftRocketEjectSocketTransform, AircraftsMesh);
				if (bLeftSuccess)
				{
					if (World)
					{
						World->SpawnActor<AAmmoEject>
							(
								RocketAmmoEjectClass,
								LeftRocketEjectSocketTransform.GetLocation(),
								LeftRocketEjectSocketTransform.GetRotation().Rotator()
							);
					}
				}
			}
		}
		bCanFireRocket = false;

		GetWorldTimerManager().SetTimer
		(
			RocketTimer,
			this,
			&AFighterAircraft::RocketFireTimerFinished,
			RocketFireDelay
		);
	}
	else
	{

	}


}

void AFighterAircraft::RocketFireTimerFinished()
{
	bCanFireRocket = true;
}
#pragma endregion
