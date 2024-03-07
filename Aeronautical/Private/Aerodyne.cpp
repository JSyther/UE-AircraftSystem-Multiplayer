// @2023 All rights reversed by Reverse-Alpha Studios


#include "Aeronautical/Aerodyne.h"

#include "Camera/CameraComponent.h"
#include "Characters/BaseCharacter.h"

#include "Components/AudioComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameModes/BaseGameMode.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystemInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"
#include "PlayerController/PlayerControllerManager.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Weapon/AmmoEject.h"

#include "Net/UnrealNetwork.h"
AAerodyne::AAerodyne()
{
	PrimaryActorTick.bCanEverTick = true;

	AreaCollision		= CreateDefaultSubobject<UBoxComponent>			(TEXT("AreaCollision"));
	AerodynesMesh		= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("HorizonRiderMesh"));
	FlapLeft			= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("FlapLeft"));
	FlapRight			= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("FlapRight"));
	RutterRight			= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("RutterRight"));
	RutterLeft			= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("RutterLeft"));
	AileronLeft			= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("AileronLeft"));
	AileronRight		= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("AileronRight"));
	ElevatorLeft		= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("ElevatorLeft"));
	ElevatorRight		= CreateDefaultSubobject<UStaticMeshComponent>	(TEXT("ElevatorRight"));

	SpringArm			= CreateDefaultSubobject<USpringArmComponent>	(TEXT("SpringArm"));
	BehindCamera		= CreateDefaultSubobject<UCameraComponent>		(TEXT("BehindCamera"));
	FrontCamera			= CreateDefaultSubobject<UCameraComponent>		(TEXT("FrontCamera"));
	InteriorCamera		= CreateDefaultSubobject<UCameraComponent>		(TEXT("InteriorCamera"));

	ExitArrow			= CreateDefaultSubobject<UArrowComponent>		(TEXT("ExitArrow"));

	SetRootComponent(AreaCollision);

	AerodynesMesh		->SetupAttachment(AreaCollision);
	FlapRight			->SetupAttachment(AerodynesMesh);
	FlapLeft			->SetupAttachment(AerodynesMesh);
	RutterRight			->SetupAttachment(AerodynesMesh);
	RutterLeft			->SetupAttachment(AerodynesMesh);
	AileronRight		->SetupAttachment(AerodynesMesh);
	AileronLeft			->SetupAttachment(AerodynesMesh);
	ElevatorRight		->SetupAttachment(AerodynesMesh);
	ElevatorLeft		->SetupAttachment(AerodynesMesh);
	ExitArrow			->SetupAttachment(AerodynesMesh);

	SpringArm			->SetupAttachment(AerodynesMesh);
	BehindCamera		->SetupAttachment(SpringArm);
	FrontCamera			->SetupAttachment(SpringArm);
	InteriorCamera		->SetupAttachment(AerodynesMesh);

	AerodynesMesh		->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	SpringArm			->TargetArmLength			= 3000.0f;
	SpringArm			->TargetOffset				= FVector(0.0f, 0.0f, 500.0f);
	SpringArm			->bEnableCameraLag			= true;
	SpringArm			->bEnableCameraRotationLag	= true;
	SpringArm			->CameraLagSpeed			= 10.0f;
	SpringArm			->CameraRotationLagSpeed	= 10.0f;

	FrontCamera			->SetRelativeLocation(FVector(5000.0f, 0.0f, 0.0f));
	FrontCamera			->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	InteriorCamera		->SetRelativeLocation(FVector(40.0F, 50.0F, 20.0F));
	InteriorCamera		->SetRelativeRotation(FRotator(0.0F, 0.0f, 0.0f));

	BehindCamera		->SetAutoActivate(true);
	FrontCamera			->SetAutoActivate(false);
	InteriorCamera		->SetAutoActivate(false);

	ExitArrow			->SetRelativeLocation(FVector(160.0f, 50.0f, 0.0f));
	ExitArrow			->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	EngineVolume		= 0.1f;
	ThrustSpeed			= MinThrustSpeedThreshold;
	CurrentSpeed		= ThrustSpeed;
	Cache_InteriorCamera = false;

	AerodynesMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	AreaCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaCollision->OnComponentBeginOverlap.AddDynamic(this, &AAerodyne::OnSphereOverlap);
	AreaCollision->OnComponentEndOverlap.AddDynamic(this, &AAerodyne::OnSphereEndOverlap);


	/*NetWorking*/
	bOnlyRelevantToOwner = false;
	bAlwaysRelevant = false;
	bReplicates = true;
	SetReplicates(true);
	SetReplicateMovement(true);

	bNetLoadOnClient = true;
	bNetUseOwnerRelevancy = false;
	NetDormancy = DORM_Awake;
	NetCullDistanceSquared = 1400000000.0f;
	NetUpdateFrequency = 90.0f;
	MinNetUpdateFrequency = 45.0f;
	NetPriority = 3.0f;

	SetPhysicsReplicationMode(EPhysicsReplicationMode::PredictiveInterpolation);

	const FRepMovement& RepMovement = GetReplicatedMovement();
	FRepMovement ModifiedRepMovement = RepMovement;
	ModifiedRepMovement.LocationQuantizationLevel = EVectorQuantization::RoundWholeNumber;
	ModifiedRepMovement.VelocityQuantizationLevel = EVectorQuantization::RoundWholeNumber;
	ModifiedRepMovement.RotationQuantizationLevel = ERotatorQuantization::ShortComponents;
}

void AAerodyne::BeginPlay()
{
	Super::BeginPlay();
	Cache_InteriorCamera = false;

	EnhancedInputLocalPlayerSubsystem();
	Handle_InitialEngine();

	/*Collision Setups when game is started*/
	if (AreaCollision != nullptr)
	{
		AreaCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AreaCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}
	else{LOG_WARNING("AREODYNE-AREACOLLISION FAILED");}
	
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AAerodyne::ReceiveDamage);
	}
}

void AAerodyne::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bPlayerEnteredVehicle && IsEngineStarted())
	{
		if (bAircraftTakenOff == false)
		{
			AutoTakeOff(DeltaTime);
		}
		else
		{
			UpdatePosition(DeltaTime);
			UpdateAxisByInputValues(StoredInputThrottle, StoredInputPitch, StoredInputYaw, StoredInputRoll);
		}
		if (bUpdateThrusters == true)
		{
			UpdateThrusters();
		}
		Play_AeroDynamicSounds();
}
	//UE_LOG(LogTemp, Warning, TEXT("AeroEngineSystem: %s"), *UEnum::GetValueAsString(AeroEngineTypes));
}

#pragma region AeroEngineTypes
void AAerodyne::OnRep_AeroEngineTypes()
{
	Handle_AeroEngineTypes();
}

void AAerodyne::CheckAeroEngineTypes()
{
	Handle_AeroEngineTypes();
}

void AAerodyne::Handle_AeroEngineTypes()
{
	switch (AeroEngineTypes)
	{
		case EAeroEngineTypes::EAET_InitialEngine:
			Handle_InitialEngine();
			break;
		case EAeroEngineTypes::EAET_EngineStarted:
			Handle_EngineStarted();
			break;
		case EAeroEngineTypes::EAET_EngineStopped:
			Handle_EngineStopped();
			break;
		case EAeroEngineTypes::EAET_Idle:
			Handle_Idle();
			break;
	}
}

void AAerodyne::Handle_InitialEngine()
{
	CurrentSpeed = 0.0f;
}

void AAerodyne::Handle_EngineStarted()
{
	SpawnTrailSystem(bMiddleEngineType, bRightEngineType, bLeftEngineType, bRightSecondEngineType, bLeftSecondEngineType);

	if (MiddleFrontThrusterFXs->IsActive() == false)
		MiddleFrontThrusterFXs->Activate();
	if (RightFrontThrusterFXs->IsActive() == false)
		RightFrontThrusterFXs->Activate();
	if (LeftFrontThrusterFXs->IsActive() == false)
		LeftFrontThrusterFXs->Activate();

	if (bUpdateThrusters == false)
	{
		bUpdateThrusters = true;
	}
	Local_OutsiteJetSound(OutsiteJetSound);
}

void AAerodyne::Handle_EngineStopped()
{
	if (MiddleFrontThrusterFXs->IsActive())
		MiddleFrontThrusterFXs->Deactivate();
	if (RightFrontThrusterFXs->IsActive())
		RightFrontThrusterFXs->Deactivate();
	if (LeftFrontThrusterFXs->IsActive())
		LeftFrontThrusterFXs->Deactivate();

	if (bUpdateThrusters == true)
	{
		bUpdateThrusters = false;
	}
}

void AAerodyne::Handle_Idle()
{

}

#pragma endregion

#pragma region Overlap

void AAerodyne::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	BaseCharacter = Cast<ABaseCharacter>(OtherActor);

	if (BaseCharacter)
	{
		BaseCharacter->Set_AerodynClass(this);
	}
}

void AAerodyne::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (BaseCharacter)
	{
		BaseCharacter->Set_AerodynClass(nullptr);
	}
}

#pragma endregion

void AAerodyne::EnhancedInputLocalPlayerSubsystem()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(AerodyneMappingContext, 0);
		}
	}
}


void AAerodyne::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{ 
		EnhancedInputComponent->BindAction(ThrottleInputAction,			 ETriggerEvent::Started,   this, &AAerodyne::InputAxis_ThrottleControl);
		EnhancedInputComponent->BindAction(ThrottleInputAction,			 ETriggerEvent::Completed, this, &AAerodyne::InputAxis_ThrottleControlReleased);

		EnhancedInputComponent->BindAction(PitchMovementInputAction,	 ETriggerEvent::Triggered, this, &AAerodyne::InputAxis_PitchControl);
		EnhancedInputComponent->BindAction(PitchMovementInputAction,	 ETriggerEvent::Completed, this, &AAerodyne::InputAxis_PitchControlReleased);

		EnhancedInputComponent->BindAction(YawMovementInputAction,		 ETriggerEvent::Started,   this, &AAerodyne::InputAxis_YawControl);
		EnhancedInputComponent->BindAction(YawMovementInputAction,		 ETriggerEvent::Completed, this, &AAerodyne::InputAxis_YawControlReleased);

		EnhancedInputComponent->BindAction(RollMovementInputAction,		 ETriggerEvent::Started,   this, &AAerodyne::InputAxis_RollControl);
		EnhancedInputComponent->BindAction(RollMovementInputAction,		 ETriggerEvent::Completed, this, &AAerodyne::InputAxis_RollControlReleased);

		EnhancedInputComponent->BindAction(BoosterInputAction,			 ETriggerEvent::Started,   this, &AAerodyne::InputAction_BoosterActivate);
		EnhancedInputComponent->BindAction(BoosterInputAction,			 ETriggerEvent::Completed, this, &AAerodyne::InputAction_BoosterDeActivate);

		EnhancedInputComponent->BindAction(ZoomInOutInputAction,		 ETriggerEvent::Triggered, this, &AAerodyne::InputAction_ZoomInOut);

		EnhancedInputComponent->BindAction(SwitchCameraInputAction,		 ETriggerEvent::Started,   this, &AAerodyne::InputAction_SwitchCamera);
		EnhancedInputComponent->BindAction(RadioInputAction,			 ETriggerEvent::Started,   this, &AAerodyne::InputAction_Radio);
		EnhancedInputComponent->BindAction(StartOrStopEngineInputAction, ETriggerEvent::Started,   this, &AAerodyne::InputAction_StartOrStopEngines);
		EnhancedInputComponent->BindAction(ExitVehicleInputAction,		 ETriggerEvent::Started,   this, &AAerodyne::InputAction_ExitVehicle);
	}
}

void AAerodyne::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAerodyne, AeroEngineTypes);
	DOREPLIFETIME(AAerodyne, OutsiteJetSound);
}

#pragma region InputFunctionalities
void AAerodyne::InputAxis_ThrottleControl(const FInputActionValue& Value)
{
	StoredInputThrottle = Value.GetMagnitude();
}

void AAerodyne::InputAxis_ThrottleControlReleased(const FInputActionValue& Value)
{
	StoredInputThrottle = Value.GetMagnitude();
}

void AAerodyne::InputAxis_PitchControl(const FInputActionValue& Value)
{
	StoredInputPitch = Value.GetMagnitude();
}

void AAerodyne::InputAxis_PitchControlReleased(const FInputActionValue& Value)
{
	StoredInputPitch = Value.GetMagnitude();;
}

void AAerodyne::InputAxis_YawControl(const FInputActionValue& Value)
{
	StoredInputYaw = Value.GetMagnitude();
}

void AAerodyne::InputAxis_YawControlReleased(const FInputActionValue& Value)
{
	StoredInputYaw = Value.GetMagnitude();;
}

void AAerodyne::InputAxis_RollControl(const FInputActionValue& Value)
{
	StoredInputRoll = Value.GetMagnitude();
}

void AAerodyne::InputAxis_RollControlReleased(const FInputActionValue& Value)
{
	StoredInputRoll = Value.GetMagnitude();;
}

/*Action Functions*/
void AAerodyne::InputAction_StartOrStopEngines()
{
	bEngineStarted = !bEngineStarted;

	if (bEngineStarted)
	{
		SetAeroEngineTypes(EAeroEngineTypes::EAET_EngineStarted);
	}
	else
	{
		SetAeroEngineTypes(EAeroEngineTypes::EAET_EngineStopped);
	}

	CheckAeroEngineTypes();
}

void AAerodyne::InputAction_BoosterActivate()
{
	if (CurrentSpeed < MaxThrustSpeed || bEngineStarted == false) return;
	if (bBoostActivated == false)
	{
		bBoostActivated = true;
	}
}

void AAerodyne::InputAction_BoosterDeActivate()
{
	if (bEngineStarted == false) return;
	if (bBoostActivated == true)
	{
		bBoostActivated = false;
	}
}

void AAerodyne::InputAction_SwitchCamera(const FInputActionValue& Value)
{
	if (BehindCamera->IsActive())
	{
		BehindCamera->SetActive(false);
		InteriorCamera->SetActive(true);

		SpringArm->bEnableCameraLag = false;
		SpringArm->bEnableCameraRotationLag = false;
		Cache_InteriorCamera = true;
	}
	else if (InteriorCamera->IsActive())
	{
		BehindCamera->SetActive(true);
		InteriorCamera->SetActive(false);

		SpringArm->bEnableCameraLag = true;
		SpringArm->bEnableCameraRotationLag = true;
		Cache_InteriorCamera = false;
	}
	else if (TargettingAerialStrikeCamera->IsActive() && Cache_InteriorCamera)
	{
		TargettingAerialStrikeCamera->Deactivate();
		BehindCamera->Deactivate();
		FrontCamera->Deactivate();
		InteriorCamera->SetActive(true);
		bCameraSwitchedWhileTargettingCameraOn = true;
	}
	else if (TargettingAerialStrikeCamera->IsActive() && Cache_InteriorCamera == false)
	{
		TargettingAerialStrikeCamera->Deactivate();
		InteriorCamera->Deactivate();
		FrontCamera->Deactivate();
		BehindCamera->SetActive(true);
		bCameraSwitchedWhileTargettingCameraOn = true;
	}
}
void AAerodyne::InputAction_ZoomInOut(const FInputActionValue& Value)
{
	if (BehindCamera->IsActive() == false) return;

	const FVector2D ZoomInputValue = Value.Get<FVector2D>();
	float ScrollInputValue = ZoomInputValue.X;

	float MinZoom = 2300.0f; // Minimum arm length
	float MaxZoom = 3000.0f; // Maximum arm length
	float CurrentArmLength = SpringArm->TargetArmLength;

	// Calculate the new zoom level
	float NewArmLength = FMath::Clamp(CurrentArmLength - ScrollInputValue * 100.0f, MinZoom, MaxZoom);
	SpringArm->TargetArmLength = NewArmLength;
}
/*Actions*/
void AAerodyne::InputAction_Radio(const FInputActionValue& Value)
{
	bRadioStarted = !bRadioStarted;

	if (bRadioStarted)
	{
		Play_Radio();
	}
	else
	{
		RadioAudioComponent->Stop();
	}
}

void AAerodyne::InputAction_ExitVehicle(const FInputActionValue& Value)
{
	if ( bPlayerEnteredVehicle == false || BaseCharacter == nullptr || ExitArrow == nullptr) return;

	if (BaseCharacter)
	{
		/*Adjusting character's properties*/
		BaseCharacter->SetActorTransform(ExitArrow->GetComponentTransform(), false, nullptr, ETeleportType::ResetPhysics);
		BaseCharacter->SetPlayerInVehicle(false);
		SetPlayerEnteredVehicle(false);

		APlayerControllerManager* PlayerControllerManager = Cast<APlayerControllerManager>(Controller);
		if (PlayerControllerManager)
		{
			PlayerControllerManager->HidePlayerHUDInVehicle(false);
		}
		APlayerController* PlayerController = Cast<APlayerController>(Controller);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			FModifyContextOptions ModifyContextOption;
			ModifyContextOption.bForceImmediately;
			Subsystem->RemoveMappingContext(AerodyneMappingContext, ModifyContextOption);

			Subsystem->AddMappingContext(BaseCharacter->GetBaseCharacterMappingContext(), 0);
		}

		if (PlayerController)
		{
			PlayerController->Possess(BaseCharacter);
		}

		bPlayerEnteredVehicle = false;
	}
}

#pragma endregion
void AAerodyne::AutoTakeOff(float DeltaTime)
{
	if (bPlayerEnteredVehicle == false || bEngineStarted == false || bAircraftTakenOff == true) return;

	AircraftTakeOffTimer += GetWorld()->GetDeltaSeconds();
	if (AircraftTakeOffTimer <= AircraftTakeOffDelay) return;
	PlayTakeOffCameraShake(TakeOffCameraShake);


	float DragMultiplier					= 1.213f;
	float AirResistenceForce				= 0.588f;
	float TakeOffThressHoldMaxSpeed			= 1500.f;
	float TargetSpeed = 
	FMath::FInterpTo
	(
		CurrentSpeed,
		TakeOffThressHoldMaxSpeed,
		DeltaTime * AirResistenceForce,
		AirDragFactor * DragMultiplier
	);

	CurrentSpeed							= TargetSpeed;

	FVector NewPosition;
	FVector ActorForwardVector				= GetActorForwardVector();
	float MultiplierSpeed					= 0.15f;
	float MultipliedCurrentSpeedbyDeltaTime = CurrentSpeed * DeltaTime;
	FVector MultipliedForwardVec			= ActorForwardVector * MultipliedCurrentSpeedbyDeltaTime;
	NewPosition								= MultipliedForwardVec;

	CalculateGravity(DeltaTime, OutGravity);

	/*Update Position*/
	float NewPosX						= NewPosition.X;
	float NewPosY						= NewPosition.Y;
	float NewPosZ						= NewPosition.Z;
	AppliedGravity						= OutGravity;

	float AffectGravity					= NewPosZ - AppliedGravity;
	FVector LastUpdatedVector			= FVector(NewPosX, NewPosY, AffectGravity);
	AddActorWorldOffset(LastUpdatedVector, true);

	float SpeedThresholdForPitchController				= 900.0f;
	float CorrectionSpeedThresholdForPitchController	= 1300.0f;
	float LastPhaseBeforeCompleteTakeOff				= 1350.0f;

	float TakeOffPitch;
	if (CurrentSpeed >= SpeedThresholdForPitchController && CurrentSpeed <= CorrectionSpeedThresholdForPitchController)
	{
		TakeOffPitch = -0.50f;
		PitchUpdate(TakeOffPitch);
	}
	else if (CurrentSpeed > CorrectionSpeedThresholdForPitchController && CurrentSpeed <= LastPhaseBeforeCompleteTakeOff)
	{
		TakeOffPitch = 1.20f;
		PitchUpdate(TakeOffPitch);
	}
	else
	{
		TakeOffPitch = 0.0f;
		PitchUpdate(TakeOffPitch);
	}

	if (CurrentSpeed > LastPhaseBeforeCompleteTakeOff + 50.0f)
	{
		bAircraftTakenOff = true;
	}
}

void AAerodyne::UpdatePosition(float DeltaTime)
{
	if (bPlayerEnteredVehicle == false || bEngineStarted == false) return;

	/*Calculate Current Speed*/
	if (ThrustSpeed < CurrentSpeed)
	{
		// If the current speed is less than the desired thrust speed,
		// interpolate towards the target speed based on air drag factor.
		float TargetSpeed = FMath::FInterpTo(CurrentSpeed, ThrustSpeed, DeltaTime, AirDragFactor);
		CurrentSpeed = TargetSpeed;
	}
	else
	{
		// If the current speed is greater than or equal to the desired thrust speed,
		// set the current speed to the thrust speed directly.
		CurrentSpeed = ThrustSpeed;
	}

	/*Calculate new position based on current speed*/
	FVector NewPosition;
	FVector ActorForwardVector				= GetActorForwardVector();
	float MultipliedCurrentSpeedbyDeltaTime = CurrentSpeed * DeltaTime;
	FVector MultipliedForwardVec			= ActorForwardVector * MultipliedCurrentSpeedbyDeltaTime;
	NewPosition								= MultipliedForwardVec;

	/*Calculate Applied Gravity*/
	float Zero								= 0.0f;

	// Adjust gravitational force based on current speed
	if (CurrentSpeed <= MinThrustSpeedThreshold / 2 + 200.0f)
	{
		GravitationalForce += 10.0f;
	}
	else if (CurrentSpeed > MinThrustSpeedThreshold)
	{
		GravitationalForce = 2000.f;
	}

	float TargetAppliedGravity =
	UKismetMathLibrary::MapRangeClamped
	(
		CurrentSpeed, 
		Zero,
		MinThrustSpeedThreshold, 
		GravitationalForce,
		Zero
	);

	AppliedGravity							= TargetAppliedGravity;
	/*Update Position*/
	float NewPosX							= NewPosition.X;
	float NewPosY							= NewPosition.Y;
	float NewPosZ							= NewPosition.Z;
	float TargetAppliedGravityFactor		= AppliedGravity * DeltaTime;
	float SubtractedZAxis					= NewPosZ - TargetAppliedGravityFactor;
	FVector LastUpdatedVector				= FVector(NewPosX, NewPosY, SubtractedZAxis);

	// Update the actor's position by adding the calculated vector to the current position.
	AddActorWorldOffset(LastUpdatedVector, true);

	// Adjust the actor's rotation if the current speed is below a certain threshold.
	if (CurrentSpeed < MinThrustSpeedThreshold)
	{
		// Define the target rotation for adjustment.
		FRotator TargetRotation = FRotator(-0.09f, -0.2f, -0.09f);

		// Interpolate towards the target rotation to smoothen the adjustment.
		FRotator NewRotation = FMath::RInterpTo(TargetRotation * 0.25f, TargetRotation, DeltaTime, 2.0f);

		// Apply the new rotation to the actor.
		AddActorWorldRotation(NewRotation);
	}

/*ReportSystem*/
	/*SpeedHack*/
	float MaxAeroDyneSpeed = 5500.0f;
	if (CurrentSpeed > MaxAeroDyneSpeed)
	{
		FString ReportedPlayerName = BaseCharacter->GetName();
		FString ReportReason = FString(TEXT("Aerodyne exceeding the speed limit"));
		ReportPlayerToServer(ReportedPlayerName, ReportReason);
	}
}

void AAerodyne::UpdateAxisByInputValues(float ThrottleValue, float PitchValue, float YawValue, float RollValue)
{
	if (bPlayerEnteredVehicle == false) return;
	ThrottleUpdate(ThrottleValue);
	PitchUpdate(PitchValue);
	YawUpdate(YawValue);
	RollUpdate(RollValue);
}

void AAerodyne::ThrottleUpdate(float InputThrottleValue)
{
	if (bBoostActivated)
	{
		float BoostIncreaseRate = 500.0f;
		BoostSpeed += BoostIncreaseRate * GetWorld()->GetDeltaSeconds();
		BoostSpeed = FMath::Clamp(BoostSpeed, 0.0f, MaxBoostSpeed);
	}

	if (bBoostActivated == false)
	{
		float BoostDecreaseRate = 750.0f; // Adjust this value as per your requirement
		BoostSpeed -= BoostDecreaseRate * GetWorld()->GetDeltaSeconds();
		BoostSpeed = FMath::Clamp(BoostSpeed, 0.0f, MaxBoostSpeed);
	}

	// Get the time passed since the last frame
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	float Zero = 0.0f;

	// Calculate the change in thrust speed based on the input throttle value
	float MultipliedAxisValuebyDeltaSeconds = InputThrottleValue * DeltaSeconds * ThrustMultiplier;
	float AddedValueToThrustSpeed = MultipliedAxisValuebyDeltaSeconds + ThrustSpeed;

	// Ensure the thrust speed does not exceed the maximum threshold
	float ClampedThrustSpeed = FMath::Clamp(AddedValueToThrustSpeed, Zero, MaxThrustSpeed);

	// Update the thrust speed
	ThrustSpeed = ClampedThrustSpeed + BoostSpeed;
}

void AAerodyne::PitchUpdate(float InputPitchValue)
{
	// Get the time passed since the last frame
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// Set the target pitch based on input value
	TargetPitch = InputPitchValue * 0.794f;

	// Interpolate towards the target pitch
	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaSeconds, AxisInterpolationSpeed);

	// Calculate the rotation for pitch
	float MultipliedPitchValueByDeltaSecond = CurrentPitch * DeltaSeconds * AeroPitchControlSpeed;
	float Zero = 0.0f;

	// Apply the pitch rotation to the actor
	FQuat PitchRotation(FVector(Zero, 1.0f, Zero), MultipliedPitchValueByDeltaSecond);
	AddActorLocalRotation(PitchRotation, false, nullptr, ETeleportType::None);
}

void AAerodyne::YawUpdate(float InputYawValue)
{
	// Get the time passed since the last frame
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// Check if the input yaw value is significant
	if (FMath::Abs(InputYawValue) > 0.01f)
	{
		// Set the target yaw based on input value
		TargetYaw = InputYawValue;

		// Interpolate towards the target yaw
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaSeconds, AxisInterpolationSpeed);

		// Calculate the rotation for yaw
		float MultipliedYawValueByDeltaSecond = CurrentYaw * DeltaSeconds * AeroYawControlSpeed;
		float Zero = 0.0f;

		// Apply the yaw rotation to the actor
		FQuat YawRotation(FVector(Zero, Zero, 1.0f), MultipliedYawValueByDeltaSecond);
		AddActorLocalRotation(YawRotation, false, nullptr, ETeleportType::None);
	}
	else
	{
		// If the input yaw value is small, interpolate towards zero yaw
		CurrentYaw = FMath::FInterpTo(CurrentYaw, 0.0f, DeltaSeconds, AxisInterpolationSpeed);

		// If the yaw is still significant, apply the rotation
		if (FMath::Abs(CurrentYaw) > SMALL_NUMBER)
		{
			float MultipliedYawValueByDeltaSecond = CurrentYaw * DeltaSeconds * AeroYawControlSpeed;
			float Zero = 0.0f;

			// Apply the yaw rotation to the actor
			FQuat YawRotation(FVector(Zero, Zero, 1.0f), MultipliedYawValueByDeltaSecond);
			AddActorLocalRotation(YawRotation, false, nullptr, ETeleportType::None);
		}
	}
}

void AAerodyne::RollUpdate(float InputRollValue)
{
	// Get the time passed since the last frame
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// Set the target roll based on input value
	TargetRoll = InputRollValue;
	AxisInterpolationSpeed = 2.0f;

	// Interpolate towards the target roll
	CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaSeconds, AxisInterpolationSpeed);

	// Calculate the rotation for roll
	float MultipliedRollValuebyDeltaSecond = CurrentRoll * DeltaSeconds * AeroRollControlSpeed;
	float Zero = 0.0f;

	// Apply the roll rotation to the actor
	FQuat RollRotation(FVector(1, Zero, Zero), MultipliedRollValuebyDeltaSecond);
	AddActorLocalRotation(RollRotation, false, nullptr, ETeleportType::None);
}

void AAerodyne::CalculateGravity(float DeltaSeconds, float AppliedGravityInput)
{
	float Zero = 0.0f;
	float GravitationalForceFactor;

	GravitationalForceFactor = FMath::GetMappedRangeValueClamped(FVector2D(Zero, MaxThrustSpeed), FVector2D(40.0f, 0.0f), CurrentSpeed);
	GravitationalForceFactor = FMath::Clamp(GravitationalForceFactor, 0.0f, 40.0f);
	AppliedGravityInput =
	UKismetMathLibrary::MapRangeClamped
	(
		CurrentSpeed,
		Zero,
		MaxThrustSpeed,
		GravitationalForceFactor,
		Zero
	);
}

#pragma region FXs
void AAerodyne::SpawnTrailSystem(bool bMiddleEngine, bool bRightEngine, bool bLeftEngine, bool bRightSecondEngine, bool bLeftSecondEngine)
{
	/*This function spawns visual effects for various thrusters attached to an aerodyne vehicle mesh, setting their properties such as scale and color to create dynamic effects.*/
	if (ThrusterFX)
	{
		if (bMiddleEngine)
		{
			MiddleFrontThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
			(
				ThrusterFX,
				AerodynesMesh,
				FName(TEXT("ThrusterMiddle")),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTargetIncludingScale,
				false,
				true,
				ENCPoolMethod::None,
				true
			);
			MiddleFrontThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 5.0f);
			MiddleFrontThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(0.0f, 0.0f, 20.0f, 1.0f));
		}		
		if (bRightEngine)
		{
			RightFrontThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
			(
				ThrusterFX,
				AerodynesMesh,
				FName(TEXT("ThrusterRight")),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTargetIncludingScale,
				false,
				true,
				ENCPoolMethod::None,
				true
			);
			RightFrontThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 5.0f);
			RightFrontThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(0.0f, 0.0f, 20.0f, 1.0f));
		}

		if (bLeftEngine)
		{
			LeftFrontThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
			(
				ThrusterFX,
				AerodynesMesh,
				FName(TEXT("ThrusterLeft")),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTargetIncludingScale,
				false,
				true,
				ENCPoolMethod::None,
				true
			);
			LeftFrontThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 5.0f);
			LeftFrontThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(0.0f, 0.0f, 20.0f, 1.0f));
		}
		//if (MiddleFrontThrusterFXs->IsActive())
		//	MiddleFrontThrusterFXs->Deactivate();
		//if (RightFrontThrusterFXs->IsActive())
		//	RightFrontThrusterFXs->Deactivate();
		//if (LeftFrontThrusterFXs->IsActive())
		//	LeftFrontThrusterFXs->Deactivate();
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ThrusterFX is not loaded!"));
		}
		//	RightThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
		//	(
		//		ThrusterFX,
		//		AerodynesMesh,
		//		FName(TEXT("ThrusterR")),
		//		FVector::ZeroVector,
		//		FRotator::ZeroRotator,
		//		EAttachLocation::SnapToTargetIncludingScale,
		//		false,
		//		true,
		//		ENCPoolMethod::None,
		//		true
		//	);
		//	RightThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 15.0f);
		//	RightThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(0.0f, 0.0f, 20.0f, 1.0f));

		//	LeftThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
		//	(
		//		ThrusterFX,
		//		AerodynesMesh,
		//		FName(TEXT("ThrusterL")),
		//		FVector::ZeroVector,
		//		FRotator::ZeroRotator,
		//		EAttachLocation::SnapToTargetIncludingScale,
		//		false,
		//		true,
		//		ENCPoolMethod::None,
		//		true
		//	);
		//	LeftThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 15.0f);
		//	LeftThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(0.0f, 0.0f, 20.0f, 1.0f));

		//	RightFrontThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
		//	(
		//		ThrusterFX,
		//		AerodynesMesh,
		//		FName(TEXT("ThrusterRF")),
		//		FVector::ZeroVector,
		//		FRotator::ZeroRotator,
		//		EAttachLocation::SnapToTargetIncludingScale,
		//		false,
		//		true,
		//		ENCPoolMethod::None,
		//		true
		//	);

		//	RightFrontThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 5.0f);
		//	RightFrontThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(20.0f, 5.0f, 5.0f, 1.0f));

		//	LeftFrontThrusterFXs = UNiagaraFunctionLibrary::SpawnSystemAttached
		//	(
		//		ThrusterFX,
		//		AerodynesMesh,
		//		FName(TEXT("ThrusterLF")),
		//		FVector::ZeroVector,
		//		FRotator::ZeroRotator,
		//		EAttachLocation::SnapToTargetIncludingScale,
		//		false,
		//		true,
		//		ENCPoolMethod::None,
		//		true
		//	);
		//}
		//LeftFrontThrusterFXs->SetVariableFloat(FName("ScaleFactorValue"), 5.0f);
		//RightFrontThrusterFXs->SetVariableLinearColor(FName("User.ScaleRGB"), FLinearColor(20.0f, 5.0f, 5.0f, 1.0f));

		//LeftThrusterFXs->Deactivate();
		//RightThrusterFXs->Deactivate();
		//LeftFrontThrusterFXs->Deactivate();
		//RightFrontThrusterFXs->Deactivate();
	}
}

void AAerodyne::UpdateThrusters()
{
	/*This function updates the visual effects for thrusters attached to an aerodyne vehicle, but only if a player has entered the vehicle.
	It calculates a mapped range value based on the current thrust speed to determine the intensity of the thruster effects.
	The calculated value is then used to set the attractor position for the thruster visual effects,
	influencing their appearance and behavior in the game world.*/

	float InRangeA		= 0.0f;
	float OutRangeA		= -10.0f;
	float OutRangeB		= -500.0f;
	float ReturnValueX	= UKismetMathLibrary::MapRangeClamped(ThrustSpeed, InRangeA, MaxThrustSpeed, OutRangeA, OutRangeB);

	FVector InValue		= FVector(ReturnValueX, 0.0f, 0.0f);

	if (MiddleFrontThrusterFXs != nullptr)
	{
		MiddleFrontThrusterFXs->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	} else {LOG_WARNING("MiddleFrontThrusterFXs = NULL")}
	if (RightFrontThrusterFXs != nullptr)
	{
		RightFrontThrusterFXs->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	} else { LOG_WARNING("RightFrontThrusterFXs = NULL") }
	if (LeftFrontThrusterFXs != nullptr)
	{
		LeftFrontThrusterFXs->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	} else { LOG_WARNING("LeftFrontThrusterFXs = NULL") }

	//LeftThrusterFXs       ->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	//RightThrusterFXs	  ->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	//LeftFrontThrusterFXs  ->SetVariableVec3(FName("User.AttractorPosition"), InValue);
	//RightFrontThrusterFXs ->SetVariableVec3(FName("User.AttractorPosition"), InValue);
}

#pragma endregion

#pragma region Sounds
void AAerodyne::Play_AeroDynamicSounds()
{
	/*This function manages the playback of aerodynamic sounds for an aerodyne vehicle based on the vehicle's state and speed.
	It adjusts the volume and pitch of engine sounds both inside and outside the vehicle, 
	as well as handling axis sounds like pitch, roll, and yaw. 
	Additionally, it spawns sound components and manages their properties to reflect the vehicle's dynamics accurately.*/

	float Zero = 0.0f;
	if (Cache_InteriorCamera) /*Inside*/
	{
		if (JetEngineSound)
		{
			if (JetEngineSound->VolumeMultiplier != 0.0f)
			{
				JetEngineSound->VolumeMultiplier = 0.0f;
			}
		}

		if (JetEngineInteriorSound)
		{
			if (bInteriorEngineSound == false)
			{
				JetEngineInteriorAudioComponent = UGameplayStatics::SpawnSoundAttached
				(
					JetEngineInteriorSound,
					GetRootComponent(),
					FName(),
					GetActorLocation(),
					EAttachLocation::KeepWorldPosition,
					false,
					1.0f,
					1.0f,
					0.0f
				);
				bInteriorEngineSound = true;
			}

			JetEngineInteriorSound->VolumeMultiplier = 0.5f;
			if (CurrentSpeed > Zero && CurrentSpeed <= MaxThrustSpeed)
			{
				EngineInteriorVolumePitch = FMath::GetMappedRangeValueClamped(FVector2D(Zero, MaxThrustSpeed), FVector2D(0.5f, 1.0f), CurrentSpeed);
				EngineInteriorVolumePitch = FMath::Clamp(EngineInteriorVolumePitch, 0.5f, 1.0f);
			}
			else if (CurrentSpeed > MaxThrustSpeed)
			{
				EngineInteriorVolumePitch = FMath::GetMappedRangeValueClamped(FVector2D(MaxThrustSpeed, MaxThrustSpeed + MaxBoostSpeed), FVector2D(1.0f, 1.3f), CurrentSpeed);
				EngineInteriorVolumePitch = FMath::Clamp(EngineInteriorVolumePitch, 1.0f, 1.3f);
			}
			JetEngineInteriorSound->PitchMultiplier = EngineInteriorVolumePitch;
		}
	}
	else  // Outside
	{
		if ( JetEngineInteriorSound && JetEngineInteriorSound->VolumeMultiplier != Zero)
		{
			 JetEngineInteriorSound->VolumeMultiplier = Zero;
			 bInteriorEngineSound = false;
		}

		if (JetEngineSound)
		{
			if (CurrentSpeed > Zero && CurrentSpeed <= MaxThrustSpeed)
			{
				EngineVolume		= FMath::GetMappedRangeValueClamped(FVector2D(Zero, MaxThrustSpeed), FVector2D(0.1f, 0.3f), CurrentSpeed);
				EngineVolume		= FMath::Clamp(EngineVolume, 0.1f, 0.3f);

				EngineVolumePitch	= FMath::GetMappedRangeValueClamped(FVector2D(Zero, MaxThrustSpeed), FVector2D(0.5f, 1.5f), CurrentSpeed);
				EngineVolumePitch	= FMath::Clamp(EngineVolumePitch, 0.5f, 1.5f);
			}
			else if (CurrentSpeed > MaxThrustSpeed) // MaxThrustSpeed + BoostSpeed 
			{
				EngineVolume		= FMath::GetMappedRangeValueClamped(FVector2D(MaxThrustSpeed, MaxThrustSpeed + MaxBoostSpeed), FVector2D(0.3f, 0.4f), CurrentSpeed);
				EngineVolume		= FMath::Clamp(EngineVolume, 0.3f, 0.4f);

				EngineVolumePitch	= FMath::GetMappedRangeValueClamped(FVector2D(MaxThrustSpeed, MaxThrustSpeed + MaxBoostSpeed), FVector2D(1.5f, 1.8f), CurrentSpeed);
				EngineVolumePitch	= FMath::Clamp(EngineVolumePitch, 1.5f, 1.8f);
			}
			JetEngineSound->VolumeMultiplier	= EngineVolume;
			JetEngineSound->PitchMultiplier		= EngineVolumePitch;
			JetEngineAudioComponent				= UGameplayStatics::SpawnSound2D(this, JetEngineSound);
		}
		if (AxisEffectSound)
		{
			if (FMath::Abs(CurrentPitch) > 1.0f)
			{
				if (bAxisSound == false)
				{

					AxisSoundEffectAudioComponent = UGameplayStatics::SpawnSoundAttached
					(
						AxisEffectSound,
						GetRootComponent(),
						FName(),
						GetActorLocation(),
						EAttachLocation::KeepWorldPosition,
						false,
						1.0f,
						1.0f,
						0.0f
					);
					bAxisSound = true;
				}
				float NormalizedPitch = FMath::Clamp(CurrentPitch, -5.0f, 5.0f) / 5.0f;

				float DefaultVolume = 0.2f;
				float MaxVolumeLevel = 0.5f;

				float DefaultPitch = 1.0f;
				float MaxPitchLevel = 1.5f;

				float InterpolationRatio = 2.5f;

				float AxisVolume = FMath::Lerp(DefaultVolume, MaxVolumeLevel, FMath::Abs(NormalizedPitch));
				float AxisPitch = FMath::Lerp(DefaultPitch, MaxPitchLevel, FMath::Abs(NormalizedPitch));

				AxisEffectSound->VolumeMultiplier = AxisVolume;
				AxisEffectSound->PitchMultiplier = AxisPitch;
			}
			else
			{
				AxisEffectSound->VolumeMultiplier = 0.0f;
				bAxisSound = false;
			}
		}
	}
}


void AAerodyne::Play_Radio()
{
	RadioAudioComponent = UGameplayStatics::SpawnSoundAttached
	(
		RadioPlaylist,
		GetRootComponent(),
		FName(),
		GetActorLocation(),
		EAttachLocation::KeepWorldPosition,
		false,
		1.0f,
		1.0f,
		0.0f
	);
}


void AAerodyne::Local_OutsiteJetSound(USoundCue* OutsiteSound)
{
	if (OutsiteJetSound && OutsiteJetSoundLoopingSoundAttenuation)
	{
		OutsiteJetSoundLoopComponent = UGameplayStatics::SpawnSoundAttached(
			OutsiteJetSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.0f,
			OutsiteJetSoundLoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
		OutsiteJetSoundLoopComponent->VolumeMultiplier = 1.0f;

		OutsiteJetSoundLoopComponent->bIsUISound = false;
		OutsiteJetSoundLoopComponent->bAutoDestroy = false;
	}
}
#pragma region 


void AAerodyne::PlayCameraShake(TSubclassOf<UCameraShakeBase> CameraShakeClass)
{
	APlayerCameraManager* PlayerCameraManager = Cast<APlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0));

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraShake(CameraShakeClass, 1);
	}
}

void AAerodyne::PlayTakeOffCameraShake(TSubclassOf<UCameraShakeBase> CameraShake)
{
	if (CameraShake)
	{
		PlayCameraShake(CameraShake);
	}
	else
	{
		LOG_WARNING("CameraShake == nullptr");
	}
}

void AAerodyne::ReportPlayerToServer(FString PlayerName, FString ReportReason)
{
	FString ReportedPlayerName	= PlayerName;
	FString ReportedReason		= ReportReason;
}

#pragma region DamageSystem
void AAerodyne::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bAerodyneDestroyed || Health <= 0) return;
	BaseGameMode = BaseGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABaseGameMode>() : BaseGameMode;

	if (BaseGameMode)
	{
		Damage = BaseGameMode->CalculateDamage(InstigatorController, Controller, Damage);
	}
	else
	{
		LOG_WARNING("BaseGameMode is nullptr, in function name ReceiveDamage at Aerodyne.cpp")
	}

	float Zero = 0.0f;
	float DamageToHealth = Damage;
	if (Shield > Zero)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, Zero, MaxShield);
			DamageToHealth = Zero;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, Zero, Damage);
			Shield = Zero;
		}
		bAeroShieldBreak = true;
	}

	if (Shield <= Zero && bAeroShieldBreak)
	{
		bAeroShieldBreak = false;
	}

	Health = FMath::Clamp(Health - DamageToHealth, Zero, MaxHealth);

	if (Health > Zero && Shield <= Zero)
	{
		PlayCameraShake(ReceiveDamageCameraShake);
	}
	if (Health <= Zero)
	{
		VehicleDestruction();
	}
}
void AAerodyne::VehicleExplosionDamage()
{
	APawn* ActorItSelf = GetInstigator();

	if (ActorItSelf)
	{
		AController* SelfController = ActorItSelf->GetController();
		if (SelfController)
		{
			if (HasAuthority())
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff
				(
					this,
					ExplosionItselfDamage,
					MinimumExplosiveDamage,
					GetActorLocation(),
					DamageInnerRadius,
					DamageOuterRadius,
					DamageFalloff,
					UDamageType::StaticClass(),
					TArray<AActor*>(),
					this,
					SelfController
				);
			}
		}
	}
}

void AAerodyne::Multicast_EnableAndSimulateAerodynPhysics_Implementation()
{
	if (AreaCollision)
	{
		AreaCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AerodynesMesh)
	{
		AerodynesMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		AerodynesMesh->SetSimulatePhysics(true);
		AerodynesMesh->SetEnableGravity(true);
		AerodynesMesh->SetSimulatePhysics(false);
		AerodynesMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	}
}

void AAerodyne::VehicleDestruction()
{
	VehicleExplosionDamage();
	
	if (ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, GetActorTransform());
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	Multicast_EnableAndSimulateAerodynPhysics();
	
	StartDestroyTimer();
}


void AAerodyne::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer
	(
		DestroyTimer,
		this,
		&AAerodyne::DestroyTimerFinished,
		DestroyTime
	);
}
void AAerodyne::DestroyTimerFinished()
{
	Destroy();
}

#pragma endregion
