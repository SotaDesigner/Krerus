// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusCombatComponent.h"

// Sets default values
AKrerusProjectile::AKrerusProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
    InitialLifeSpan = 10.f;

	Collider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	RootComponent = Collider;

	Collider->InitSphereRadius(15.f);
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collider->SetCollisionResponseToAllChannels(ECR_Block);
	Collider->OnComponentHit.AddDynamic(this, &AKrerusProjectile::OnProjectileHit);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	ProjectileMovement->SetUpdatedComponent(Collider);
	ProjectileMovement->InitialSpeed = 1200.f;
	ProjectileMovement->MaxSpeed = 1200.f;
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 6000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	bReplicates = true;
	SetReplicateMovement(true);
}

void AKrerusProjectile::InitializeProjectile(AActor* target, float damage, UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffectClass,
    TSubclassOf<UGameplayEffect> InResourceEffectClass, float InAbilityLevel)
{
	if (!IsValid(target) || !IsValid(target->GetRootComponent()))
	{
		return;
	}

	Target = target;
	
	ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();

	ProjectileDamage = damage;
	ASC = InSourceASC;
	DamageEffectClass = InDamageEffectClass;
    ResourceEffectClass = InResourceEffectClass;
    SourceAbilityLevel = InAbilityLevel;
}

void AKrerusProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!HasAuthority() || bHitResolved || OtherActor == GetInstigator()) return;
    bHitResolved = true;
    Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMovement->StopMovementImmediately();

    if (OtherActor == Target.Get())
    {
        auto* Character = Cast<AKrerusCharacter>(GetInstigator());
        auto* Enemy = Cast<AKrerusEnemy>(OtherActor);
        auto* Combat = IsValid(Character) ? Character->GetCombatComponent() : nullptr;
        if (Combat && IsValid(Enemy))
            Combat->ResolveAutoAttackHit(Enemy, ProjectileDamage, ASC.Get(), DamageEffectClass,
                ResourceEffectClass, SourceAbilityLevel);
    }

	Destroy();
}

// Called when the game starts or when spawned
void AKrerusProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetInstigator()))
	{
		Collider->IgnoreActorWhenMoving(GetInstigator(), true);
	}
    // Clients display server movement; they do not simulate impacts or local homing.
    if (!HasAuthority())
    {
        Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ProjectileMovement->Deactivate();
    }
}

// Called every frame
void AKrerusProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

