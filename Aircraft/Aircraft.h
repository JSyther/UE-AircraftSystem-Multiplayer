// @2023 All rights reversed

/**
 * The AAircraft class represents the base class for aircraft vehicles in the game.
 * Aircraft vehicles encompass a variety of flying vehicle types, including fighters, transports, and other aerial crafts.
 * This class inherits functionality from the APawn class and serves as a foundation for specific aircraft types.
 * It provides basic attributes and functionalities common to all aircraft vehicles, such as engine management and input handling.
 * Derived classes can extend and specialize the behavior of aircraft vehicles according to their specific roles and purposes.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"

#include "Aircraft.generated.h"


#define TRACE_MAX_LENGTH 80000.0f
#define LOG_WARNING(Format)  UE_LOG(LogTemp, Warning, TEXT(Format));
#pragma region Pre-Classes&Components

class ABaseCharacter;
class ABaseGameMode;
class AController;
class UAudioComponent;
class UArrowComponent;
class UCameraComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UBoxComponent;
class USoundCue;
class UNiagaraComponent;
class UNiagaraSystem;

class UInputMappingContext;
class UInputAction;
#pragma endregion

UENUM(BlueprintType)
enum class EAircraftEngineTypes : uint8
{
	EACET_InitialEngine		UMETA(DisplayName = "Initial"),
	EACET_EngineStarted		UMETA(DisplayName = "EngineStarted"),
	EACET_EngineStopped		UMETA(DisplayName = "EngineStopped"),
	EACET_Idle				UMETA(DisplayName = "Idle"),

	EACET_MAX				UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class SYNTHETICA_API AAircraft : public APawn
{
	GENERATED_BODY()

#pragma region General
public:
	AAircraft();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

#pragma endregion

#pragma region OtherClasses
	/*Variables*/
private:
	UPROPERTY(ReplicatedUsing = OnRep_AircraftEngineTypes)
	EAircraftEngineTypes AircraftEngineTypes = EAircraftEngineTypes::EACET_InitialEngine;

private:
	UFUNCTION()
	void OnRep_AircraftEngineTypes();
	void Handle_AircraftEngineTypes();
	void Handle_InitialEngine();
	void Handle_EngineStarted();
	void Handle_EngineStopped();
	void Handle_Idle();

public:
	EAircraftEngineTypes GetAircraftEngineTypes() const { return AircraftEngineTypes; }
	void			 SetAircraftEngineTypes(EAircraftEngineTypes Type) { AircraftEngineTypes = Type; }

	void CheckAircraftEngineTypes();
#pragma endregion

#pragma region Overlapping
private:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
#pragma endregion

#pragma region Components
private:
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			StartOrStopEngineInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)			
	UInputMappingContext*	AircraftMappingContext;
	UPROPERTY(EditAnywhere, Category = InputSettings)	
	UInputAction*			ThrottleInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			PitchMovementInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			YawMovementInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			RollMovementInputAction;

	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			BoosterInputAction;

	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			SwitchCameraInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)			
	UInputAction*			ZoomInOutInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			RadioInputAction;
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction*			ExitVehicleInputAction;

protected:
	UPROPERTY(EditAnywhere)		UArrowComponent*	  ExitArrow;
	UPROPERTY(EditAnywhere)		UBoxComponent*		  AreaCollision;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* AircraftMesh;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* FlapLeft;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* FlapRight;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* RudderRight;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* RudderLeft;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* AileronLeft;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* AileronRight;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* ElevatorLeft;
	UPROPERTY(EditAnywhere)		UStaticMeshComponent* ElevatorRight;
	
	UPROPERTY(VisibleAnywhere)	USpringArmComponent*  SpringArm;
	UPROPERTY(VisibleAnywhere)	UCameraComponent*	  BehindCamera;
	UPROPERTY(VisibleAnywhere)	UCameraComponent*	  FrontCamera;
	UPROPERTY(VisibleAnywhere)	UCameraComponent*	  InteriorCamera;
	UPROPERTY(VisibleAnywhere)	UCameraComponent*	  TargetingAerialStrikeCamera;


private:
	UPROPERTY()
	ABaseCharacter* BaseCharacter = nullptr;

	UPROPERTY()
	ABaseGameMode* BaseGameMode;

public:
	FORCEINLINE UInputMappingContext* Get_AircraftMappingContext() const { return AircraftMappingContext; }
#pragma endregion

#pragma region Inputs
private:
	void EnhancedInputLocalPlayerSubsystem();

/*Axis Functions*/

	void InputAxis_ThrottleControl			(const FInputActionValue& Value);
	void InputAxis_ThrottleControlReleased  (const FInputActionValue& Value);

	void InputAxis_PitchControl				(const FInputActionValue& Value);
	void InputAxis_PitchControlReleased		(const FInputActionValue& Value);

	void InputAxis_YawControl				(const FInputActionValue& Value);
	void InputAxis_YawControlReleased		(const FInputActionValue& Value);

	void InputAxis_RollControl				(const FInputActionValue& Value);
	void InputAxis_RollControlReleased		(const FInputActionValue& Value);

/*Action Functions*/
	void InputAction_StartOrStopEngines		(	/*EngineFunction*/		   );

	void InputAction_BoosterActivate		(	/*Booster Activate*/	   );
	void InputAction_BoosterDeactivate		(							   );

	void InputAction_SwitchCamera			(const FInputActionValue& Value);
	void InputAction_ZoomInOut				(const FInputActionValue& Value);

	void InputAction_Radio					(const FInputActionValue& Value);
	void InputAction_ExitVehicle			(const FInputActionValue& Value);

// StoredVariables
	float StoredInputThrottle;
	float StoredInputPitch;
	float StoredInputYaw;
	float StoredInputRoll;

/*Functions*/
	void AutoTakeOff				(float DeltaTime);
	void UpdatePosition				(float DeltaTime);

	void UpdateAxisByInputValues	(float ThrottleValue, float PitchValue, float YawValue, float RollValue);
	void ThrottleUpdate				(float InputThrottleValue);
	void PitchUpdate				(float InputPitchValue);
	void YawUpdate					(float InputYawValue);
	void RollUpdate					(float InputRollValue);

	void CalculateGravity			(float DeltaSeconds, float AppliedGravityInput);
	float OutGravity;

/*Booleans*/
private:
	bool bEngineStarted			= false;
	bool bPlayerEnteredVehicle	= false;
	bool bBoostActivated		= false;
	bool bUpdateThrusters		= false;
	bool bAircraftTakenOff		= false;


/*AircraftTakeOff*/
	float AircraftTakeOffTimer;
	UPROPERTY(EditAnywhere)
	float AircraftTakeOffDelay = 2.0f;
/*Getter and Setters*/
public:
	bool GetPlayerEnteredVehicle()	 const { return bPlayerEnteredVehicle; }
	bool IsEngineStarted()			 const { return bEngineStarted; }
	bool IsAircraftBoostActivated() const { return bBoostActivated; }

	void SetPlayerEnteredVehicle(bool bPlayerEnter) { bPlayerEnteredVehicle = bPlayerEnter; }
	void StartEngines(bool bStart) { bEngineStarted = bStart; }
#pragma endregion

#pragma region Movement-Probs
protected:
/*Dynamics*/
	float ThrustSpeed;
	float CurrentSpeed;
	float AppliedGravity;
	float BoostSpeed;

/*Rotation*/
	float TargetYaw;
	float CurrentYaw;
	float TargetPitch;
	float CurrentPitch;
	float TargetRoll;
	float CurrentRoll;

protected:
/*Editables*/
	UPROPERTY(EditAnywhere)
	float MaxThrustSpeed			= 4000.0f;

	UPROPERTY(EditAnywhere)
	float MinThrustSpeedThreshold	= 1000.0f;

	UPROPERTY(EditAnywhere)
	float ThrustMultiplier			= 1000.0f;

	UPROPERTY(EditAnywhere)
	float MaxBoostSpeed				= 1500.0f;

	UPROPERTY(EditAnywhere)
	float GravitationalForce		= 2000.0f;

	UPROPERTY(EditAnywhere)
	float AirDragFactor				= 0.5f;

/*ControlSurfaces*/
	UPROPERTY(EditAnywhere)
	float MaxFlapPitch				= 150.0f;

	UPROPERTY(EditAnywhere)
	float MaxElevatorPitch			= 25.0f;

	UPROPERTY(EditAnywhere)
	float MaxRudderYaw				= 45.0f;

	UPROPERTY(EditAnywhere)
	float MaxAileronPitch			= 45.0f;

/*AxisControllerValues*/
	UPROPERTY(EditAnywhere)
	float AircraftPitchControlSpeed		= 0.25f;
	UPROPERTY(EditAnywhere)
	float AircraftYawControlSpeed		= 0.50f;
	UPROPERTY(EditAnywhere)
	float AircraftRollControlSpeed		= 1.50f;
#pragma endregion

#pragma region Adjustable Properties
private:
UPROPERTY(EditAnywhere)
float AxisInterpolationSpeed = 1.0f;

#pragma endregion

#pragma region Camera
private:
UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
TSubclassOf<UCameraShakeBase> TakeOffCameraShake;

UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
TSubclassOf<UCameraShakeBase> ReceiveDamageCameraShake;

void PlayCameraShake(TSubclassOf<UCameraShakeBase> CameraShakeClass);
void PlayTakeOffCameraShake(TSubclassOf<UCameraShakeBase> CameraShake);

protected:
	bool Cache_InteriorCamera = false;
	bool bCameraSwitchedWhileTargetingCameraOn = false;
#pragma endregion

#pragma region FXs
protected:
	UPROPERTY(EditAnywhere)
	bool bMiddleEngineType = false;

	UPROPERTY(EditAnywhere)
	bool bRightEngineType = false;

	UPROPERTY(EditAnywhere)
	bool bLeftEngineType = false;

	UPROPERTY(EditAnywhere)
	bool bRightSecondEngineType = false;

	UPROPERTY(EditAnywhere)
	bool bLeftSecondEngineType = false;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem*		ThrusterFX;

	UPROPERTY()
	UNiagaraComponent*	MiddleThrusterFXs;

	UPROPERTY()
	UNiagaraComponent*	RightThrusterFXs;

	UPROPERTY()
	UNiagaraComponent*	LeftThrusterFXs;

	UPROPERTY()
	UNiagaraComponent*  MiddleFrontThrusterFXs;

	UPROPERTY()
	UNiagaraComponent*	RightFrontThrusterFXs;

	UPROPERTY()
	UNiagaraComponent*	LeftFrontThrusterFXs;

	virtual void SpawnTrailSystem(bool bMiddleEngine, bool bRightEngine, bool bLeftEngine, bool bRightSecondEngine, bool bLeftSecondEngine);
	void UpdateThrusters();
#pragma endregion

#pragma region Sounds
/*JetEngine*/
	UAudioComponent* JetEngineAudioComponent		 = nullptr;
	UAudioComponent* JetEngineInteriorAudioComponent = nullptr;
	UAudioComponent* AxisSoundEffectAudioComponent	 = nullptr;

	/*FlightSystems Probs*/
	UPROPERTY(EditAnywhere)
	USoundCue* JetEngineSound;

	UPROPERTY(EditAnywhere)
	USoundCue* JetEngineInteriorSound;

	UPROPERTY(EditAnywhere)
	USoundCue* AxisEffectSound;

	float EngineVolume		= 0.1f;
	float EngineVolumePitch = 0.5f;
	float EngineInteriorVolumePitch;
	float InteriorEngineVolume;
	float InteriorVolumePitch;

	bool bInteriorEngineSound;
	bool bEngineSound;
	bool bAxisSound = false;


	void Play_AerodynamicSounds();
	void Local_OutsideJetSound(USoundCue* OutsideSound);

/*Radio*/
	bool bRadioStarted = false;

	UPROPERTY(EditAnywhere)
	USoundCue* RadioPlaylist;

	UPROPERTY()
	UAudioComponent* RadioAudioComponent;

	void Play_Radio();
#pragma endregion

#pragma region Replications
private:
	UPROPERTY(EditAnywhere, Category = "ReplicationSound", Replicated)
	USoundCue* OutsideJetSound;

	UPROPERTY()
	UAudioComponent* OutsideJetSoundLoopComponent;

	UPROPERTY(EditAnywhere, Category = "ReplicationSound")
	USoundAttenuation* OutsideJetSoundLoopingSoundAttenuation;

#pragma endregion

#pragma region ReportSystems
private:
	void ReportPlayerToServer(FString PlayerName, FString ReportReason);
#pragma endregion

#pragma region Attributes - Stats
private:
	bool bAircraftDestroyed = false;
	bool bAircraftShieldBreak	= false;

	float Health		= 500.0f;
	float Shield		= 500.0f;
	float MaxHealth		= 500.0f;
	float MaxShield		= 500.0f;

	float BoosterFuel	= 1500.0f;
#pragma endregion

#pragma region Damage&Destruction-System
private:
	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	class UParticleSystem* ExplosionParticle;

	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	class USoundCue*	   ExplosionSound;

	UFUNCTION()
	void ReceiveDamage
	(
		AActor* DamagedActor, 
		float Damage, 
		const UDamageType* DamageType,
		AController* InstigatorController, 
		AActor* DamageCauser
	);

	void VehicleExplosionDamage();
	void VehicleDestruction();
/*Numerical*/
	float ExplosionItselfDamage		 = FMath::RandRange(100.0f, 125.0f);
	float MinimumExplosiveDamage	 = FMath::RandRange(50.0f, 100.0f);

	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	float DamageInnerRadius			 = 500.0f;

	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	float DamageOuterRadius			 = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	float DamageFalloff				 = 1.0f;

/*Destroy Timer*/
private:
	FTimerHandle DestroyTimer;
	UPROPERTY(EditAnywhere, Category = "DeveloperProperties")
	float DestroyTime = 5.0f;
	void StartDestroyTimer();
	void DestroyTimerFinished();

	/*Server & Multicast*/
	/*Replications*/
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EnableAndSimulateAircraftPhysics();

#pragma endregion
};
