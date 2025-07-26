// @2023 All rights reversed by Reverse-Alpha Studios


#include "Weapon/ProjectileRocket.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/RocketMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "Sound/SoundCue.h"
#include "Aeronautical/Aerodyne.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->InitialSpeed	= 5000.f;
	RocketMovementComponent->MaxSpeed		= 7000.0f;
	RocketMovementComponent->ProjectileGravityScale = 0.05f;
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}
	
	SpawnTrailSystem();
	if (RocketProjectileLoop && RocketProjectileLoopingSoundAttenuation)
	{
		RocketProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached
		(
			RocketProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.0f,
			RocketProjectileLoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	if (OtherActor->IsA(AAerodyne::StaticClass()))
	{

		float MinRocketDamage = 150.0f;
		float MaxRocketDamage = 450.0f;
		float RandomDamageRate = FMath::FRandRange(MinRocketDamage, MaxRocketDamage);

		UGameplayStatics::ApplyDamage
		(
			OtherActor,
			RandomDamageRate,
			GetOwner()->GetInstigatorController(),
			this,
			UDamageType::StaticClass()
		);
	}

	ExplodeDamage();
	StartDestroyTimer();

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}

	if (RocketProjectileLoopComponent && RocketProjectileLoopComponent->IsPlaying())
	{
		RocketProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::Destroyed()
{

}
