// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusEnemy.h"
#include "KrerusAttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "KrerusPlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AKrerusEnemy::AKrerusEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void AKrerusEnemy::OnEnemyHealthChanged(float CurrentHealth, float MaxHealth)
{
    if (CurrentHealth < PreviousHealth && CurrentHealth > 0.0f)
    {
        PlayHitReaction();
    }

    PreviousHealth = CurrentHealth;
}

// Called when the game starts or when spawned
void AKrerusEnemy::BeginPlay()
{
	Super::BeginPlay();
	
    if (IsValid(AttributeComponent))
    {
        PreviousHealth = AttributeComponent->GetCurrentHealth();
        AttributeComponent->OnHealthChanged.AddUniqueDynamic(this,&AKrerusEnemy::OnEnemyHealthChanged);
    }

    AttributeComponent->OnDeath.AddUniqueDynamic(this,&AKrerusEnemy::OnEnemyDeath);
}

// Called every frame
void AKrerusEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AKrerusEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AKrerusEnemy::PlayHitReaction()
{
    if (!IsValid(AttributeComponent) || !AttributeComponent->IsAlive())
    {
        return;
    }

    if (!IsValid(HitReactionMontage))
    {
        return;
    }

    USkeletalMeshComponent* EnemyMesh = GetMesh();

    if (!IsValid(EnemyMesh))
    {
        return;
    }

    UAnimInstance* AnimInstance = EnemyMesh->GetAnimInstance();

    if (!IsValid(AnimInstance))
    {
        return;
    }

    if (AnimInstance->Montage_IsPlaying(HitReactionMontage))
    {
        return;
    }

    AnimInstance->Montage_Play(HitReactionMontage);
}

void AKrerusEnemy::OnEnemyDeath()
{
    if (!IsValid(DeathMontage))
    {
        return;
    }

    USkeletalMeshComponent* EnemyMesh = GetMesh();

    if (!IsValid(EnemyMesh))
    {
        return;
    }

    UAnimInstance* AnimInstance = EnemyMesh->GetAnimInstance();

    if (!IsValid(AnimInstance))
    {
        return;
    }

    AnimInstance->Montage_Play(DeathMontage);

    UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Enemigo %s ha muerto"),*GetName());

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    }

    GetWorldTimerManager().SetTimer(CorpseTimerHandle,this,&AKrerusEnemy::OnCorpseTimeExpired,CorpseDuration,false);
}

bool AKrerusEnemy::IsCorpse() const
{
    if (!IsValid(AttributeComponent))
    {
        return false;
    }

    return !AttributeComponent->IsAlive();
}

void AKrerusEnemy::OnCorpseTimeExpired()
{
    UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Ha terminado el tiempo del cadaver de %s"),*GetName());

    // Limpiar la selección si este enemigo está seleccionado.
    if (AKrerusPlayerController* PlayerController = Cast<AKrerusPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        PlayerController->ClearSelectionIfTarget(this);
    }

    // Ocultar al enemigo.
    SetActorHiddenInGame(true);

    // Desactivar todas sus colisiones.
    SetActorEnableCollision(false);
}