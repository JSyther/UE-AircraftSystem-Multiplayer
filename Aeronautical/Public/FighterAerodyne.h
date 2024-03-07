// @2023 All rights reversed by Reverse-Alpha Studios
/**
 * AFighterAerodyne represents a class of high-performance aircraft designed primarily for combat and aerial engagement.
 * It inherits functionality and characteristics from the AAerodyne base class, but it is specialized for combat purposes.
 * Typically equipped with advanced weaponry and maneuvering capabilities, AFighterAerodyne excels in aerial battles and dogfights.
 */

#pragma once

#include "CoreMinimal.h"
#include "Aeronautical/Aerodyne.h"
#include "FighterAerodyne.generated.h"

class UAnimationAsset;
class AAmmoEject;
class AProjectile;
class AProjectileRocket;
class USoundCue;

UCLASS()
class SYNTHETICA_API AFighterAerodyne : public AAerodyne
{
	GENERATED_BODY()
	
protected:
	AFighterAerodyne();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	float ResetDataTimer;
#pragma region Inputs
private:
	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction* FireRocketInputAction;

	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction* FireProjectileInputAction;

	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction* SwitchTurretModeInputAction;

	UPROPERTY(EditAnywhere, Category = InputSettings)
	UInputAction* SwitchExplosiveModeInputAction;

/*Input Functions*/
	void InputAction_SwitchTurretMode();

	void InputAction_SwitchExplosiveMode();

/*Variables*/
public:
	bool bMultiTurret = true;
	bool bRocketMode = true;

/*Camera*/

	void CheckAndFixTargettingCameraModeIfSwitched();
#pragma endregion

#pragma region Fire-Systems
private:
	bool bCanFireRocket = true;
	bool bCanFireTurret = true;

	void FireTurret();
	void LaunchRocket();

	void SingleFireTurretEnd();

	void TurretTimerFinished();
	void RocketFireTimerFinished();

	FTimerHandle TurretTimer;
	FTimerHandle RocketTimer;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	TSubclassOf<AProjectileRocket> ProjectileRocketClass;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Developer Properties")
	TSubclassOf<AAmmoEject> RocketAmmoEjectClass;


	/*TODO : optional weapon animation play*/
	UPROPERTY(EditAnywhere, Category = "Developer Properties")
	UAnimationAsset*		BulletFireAnimation;

	UPROPERTY(EditAnywhere, Category = "Developer Properties")
	UAnimationAsset*		RocketFireAnimation;


	/*AdjustableForDevelopers*/
protected:
	UPROPERTY(EditAnywhere)
	float TurretFireDelay = 0.10f;

	UPROPERTY(EditAnywhere)
	float RocketFireDelay = 7.5f;
#pragma endregion

#pragma region Sounds
private:
	UPROPERTY(EditAnywhere)
	USoundCue* TurretFireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* SingleTurretFireSoundStart;

	UPROPERTY(EditAnywhere)
	USoundCue* SingleTurretFireSoundEnd;
#pragma endregion
};
