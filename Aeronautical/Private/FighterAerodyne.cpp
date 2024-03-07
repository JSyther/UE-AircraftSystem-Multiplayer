// @2023 All rights reversed by Reverse-Alpha Studios


#include "Aeronautical/FighterAerodyne.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/RocketMovementComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

#include "Weapon/Projectile.h"
#include "Weapon/ProjectileRocket.h"
#include "TimerManager.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AFighterAerodyne::AFighterAerodyne()
{
	FVector FighterAerodyneBoxExtent(600.0f, 425.0f, 100.0f);
	AreaCollision->SetBoxExtent(FighterAerodyneBoxExtent);

	TargettingAerialStrikeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TargettingCamera"));
	TargettingAerialStrikeCamera->SetupAttachment(AerodynesMesh);
	TargettingAerialStrikeCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

void AFighterAerodyne::BeginPlay()
{
	Super::BeginPlay();
	TargettingAerialStrikeCamera->SetActive(false);
}

void AFighterAerodyne::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UStaticMeshSocket* RocketRightSocket = AerodynesMesh->GetSocketByName(FName("RocketRight"));

	UWorld* World = GetWorld();
	if (RocketRightSocket && World)
	{
		FTransform RightSocketTransform;
		bool bRightSuccess = RocketRightSocket->GetSocketTransform(RightSocketTransform, AerodynesMesh);
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

void AFighterAerodyne::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	AAerodyne::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(FireRocketInputAction,			ETriggerEvent::Triggered,	this, &AFighterAerodyne::LaunchRocket);
		EnhancedInputComponent->BindAction(FireProjectileInputAction,		ETriggerEvent::Triggered,	this, &AFighterAerodyne::FireTurret);
		EnhancedInputComponent->BindAction(SwitchTurretModeInputAction,		ETriggerEvent::Started,		this, &AFighterAerodyne::InputAction_SwitchTurretMode);
		EnhancedInputComponent->BindAction(SwitchExplosiveModeInputAction,	ETriggerEvent::Started,		this, &AFighterAerodyne::InputAction_SwitchExplosiveMode);
		EnhancedInputComponent->BindAction(FireProjectileInputAction,		ETriggerEvent::Completed,	this, &AFighterAerodyne::SingleFireTurretEnd);
	}
}

#pragma region Inputs
void AFighterAerodyne::InputAction_SwitchTurretMode()
{
	bMultiTurret = !bMultiTurret;
}

void AFighterAerodyne::InputAction_SwitchExplosiveMode()
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
void AFighterAerodyne::CheckAndFixTargettingCameraModeIfSwitched()
{
	if (bCameraSwitchedWhileTargettingCameraOn)
	{
		bRocketMode = !bRocketMode;
		bCameraSwitchedWhileTargettingCameraOn = false;
	}
}
#pragma endregion

#pragma region Fire-Systems



void AFighterAerodyne::FireTurret()
{
	/*This function, FireTurret(), is responsible for firing turrets on a fighter aerodyne object. It first checks if the turret can fire and if an aerial strike camera is not active. Depending on whether the aerodyne has multiple turrets or not, it calculates the firing direction and spawns projectiles accordingly, accompanied by appropriate sound effects. After firing, it sets a delay before the turret can fire again and logs various checkpoints for debugging purposes. */
	if (bCanFireTurret == false || TargettingAerialStrikeCamera->IsActive()) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();

	if (bMultiTurret)
	{
		if (TurretFireDelay != 0.10f)
		{
			TurretFireDelay = 0.10f;
		}

		const UStaticMeshSocket* TurretRightSocket = AerodynesMesh->GetSocketByName(FName("TurretRight"));
		if (World && TurretRightSocket)
		{
			FTransform RightTurretTransform;
			bool bRightSuccess = TurretRightSocket->GetSocketTransform(RightTurretTransform, AerodynesMesh);
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
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(TurretFireSound, AerodynesMesh, FName("TurretRight"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(RightTurretTransform.GetLocation(), RightTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, RightTurretTransform.GetLocation());
			}
		}

		const UStaticMeshSocket* TurretLeftSocket = AerodynesMesh->GetSocketByName(FName("TurretLeft"));
		if (World && TurretLeftSocket)
		{
			FTransform LeftTurretTransform;
			bool bRightSuccess = TurretLeftSocket->GetSocketTransform(LeftTurretTransform, AerodynesMesh);
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
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(TurretFireSound, AerodynesMesh, FName("TurretLeft"));
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
		const UStaticMeshSocket* TurretMiddleSocket = AerodynesMesh->GetSocketByName(FName("TurretMiddle"));
		if (World && TurretMiddleSocket)
		{
			FTransform MiddleTurretTransform;
			bool bMiddleSuccess = TurretMiddleSocket->GetSocketTransform(MiddleTurretTransform, AerodynesMesh);
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
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(SingleTurretFireSoundStart, AerodynesMesh, FName("TurretMiddle"));
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
		&AFighterAerodyne::TurretTimerFinished,
		TurretFireDelay
	);
}

void AFighterAerodyne::SingleFireTurretEnd()
{
	if (bMultiTurret == true || TargettingAerialStrikeCamera->IsActive()) return;
	if (SingleTurretFireSoundEnd != nullptr)
	{
		const UStaticMeshSocket* TurretMiddleSocket = AerodynesMesh->GetSocketByName(FName("TurretMiddle"));
		if (TurretMiddleSocket)
		{
			FTransform MiddleTurretTransform;
			bool bMiddleSuccess = TurretMiddleSocket->GetSocketTransform(MiddleTurretTransform, AerodynesMesh);
			if (bMiddleSuccess)
			{
				UAudioComponent* SoundComponent = UGameplayStatics::SpawnSoundAttached(SingleTurretFireSoundEnd, AerodynesMesh, FName("TurretMiddle"));
				if (SoundComponent)
				{
					SoundComponent->SetWorldLocationAndRotation(MiddleTurretTransform.GetLocation(), MiddleTurretTransform.GetRotation());
				}
				//UGameplayStatics::PlaySoundAtLocation(World, TurretFireSound, LeftTurretTransform.GetLocation());
			}
		}
	}
}

void AFighterAerodyne::TurretTimerFinished()
{
	bCanFireTurret = true;
}

void AFighterAerodyne::LaunchRocket()
{
	if (bRocketMode)
	{
		if (bCanFireRocket == false) return;

		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		UWorld* World = GetWorld();
		const UStaticMeshSocket* RocketRightSocket = AerodynesMesh->GetSocketByName(FName("RocketRight"));
		if (World && RocketRightSocket)
		{
			FTransform RightSocketTransform;
			bool bRightSuccess = RocketRightSocket->GetSocketTransform(RightSocketTransform, AerodynesMesh);
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

		const UStaticMeshSocket* RocketLeftSocket = AerodynesMesh->GetSocketByName(FName("RocketLeft"));
		if (World && RocketLeftSocket)
		{
			FTransform LeftSocketTransform;
			bool bLeftSuccess = RocketLeftSocket->GetSocketTransform(LeftSocketTransform, AerodynesMesh);
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
			const UStaticMeshSocket* RightRocketAmmoEjectSocket = AerodynesMesh->GetSocketByName(FName("RocketAmmoEjectRight"));
			if (RightRocketAmmoEjectSocket != nullptr)
			{
				FTransform RightRocketEjectSocketTransform;
				bool bRightSuccess = RightRocketAmmoEjectSocket->GetSocketTransform(RightRocketEjectSocketTransform, AerodynesMesh);
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

			const UStaticMeshSocket* LeftRocketAmmoEjectSocket = AerodynesMesh->GetSocketByName(FName("RocketAmmoEjectLeft"));
			if (LeftRocketAmmoEjectSocket != nullptr)
			{
				FTransform LeftRocketEjectSocketTransform;
				bool bLeftSuccess = LeftRocketAmmoEjectSocket->GetSocketTransform(LeftRocketEjectSocketTransform, AerodynesMesh);
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
			&AFighterAerodyne::RocketFireTimerFinished,
			RocketFireDelay
		);
	}
	else
	{

	}


}

void AFighterAerodyne::RocketFireTimerFinished()
{
	bCanFireRocket = true;
}
#pragma endregion