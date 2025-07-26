#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class AIRCRAFT_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay()			override;
	virtual void Tick(float DeltaTime)	override;
	virtual void Destroyed()			override;

	void ExplodeDamage();

	void SpawnTrailSystem();
	void StartDestroyTimer();
	void DestroyTimerFinished();


protected:
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;


	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	UParticleSystem* ProjectileTracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float MinimumDamage	= 10.0f;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float DamageInnerRadius	= 200.0f;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float DamageOuterRadius	= 500.f;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float DamageFalloff	= 1.0f;

public:
	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float HeadShotDamage = 40.0f;

protected:
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere, Category = "WeaponSettings")
	float DestroyTime = 3.0f;

#pragma region ServerSide-Rewind
protected:
	bool bUseServerSideRewind = false;

	UPROPERTY(EditAnywhere)
	float ProjectileSpeed = 15000;

public:
	FVector_NetQuantize			TraceStart;
	FVector_NetQuantize100		InitialVelocity;

public:
	FORCEINLINE bool ServerSideRewind() const { return bUseServerSideRewind; }
	FORCEINLINE void SetServerSideRewind(bool SetValue)  { bUseServerSideRewind = SetValue; }
	FORCEINLINE float GetProjectileSpeed() const { return ProjectileSpeed; }
#pragma endregion
};
