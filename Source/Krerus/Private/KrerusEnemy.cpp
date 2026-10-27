#include "KrerusEnemy.h"
#include "KrerusAttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "KrerusPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.h"

AKrerusEnemy::AKrerusEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    AbilitySystemComponent->SetIsReplicated(true);

    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

    GASAttributeSet = CreateDefaultSubobject<UKrerusAttributeSet>(TEXT("GASAttributeSet"));
}

void AKrerusEnemy::OnEnemyHealthChanged(float CurrentHealth, float MaxHealth)
{
    if (CurrentHealth < PreviousHealth && CurrentHealth > 0.0f)
    {
        PlayHitReaction();
    }

    PreviousHealth = CurrentHealth;
}

// Suscribe reacciones y muerte a los atributos heredados del NPC; inicializa aparte el ASC propio del enemigo.
void AKrerusEnemy::BeginPlay()
{
    Super::BeginPlay();

    if (IsValid(AttributeComponent))
    {
        PreviousHealth = AttributeComponent->GetCurrentHealth();
        AttributeComponent->OnHealthChanged.AddUniqueDynamic(this, &AKrerusEnemy::OnEnemyHealthChanged);
    }

    AttributeComponent->OnDeath.AddUniqueDynamic(this, &AKrerusEnemy::OnEnemyDeath);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

void AKrerusEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

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

// Evento de muerte tradicional: reproduce el montaje y programa el tiempo de permanencia del cadáver.
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

    UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Enemigo %s ha muerto"), *GetName());

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }

    GetWorldTimerManager().SetTimer(CorpseTimerHandle, this, &AKrerusEnemy::OnCorpseTimeExpired, CorpseDuration, false);
}

// Consulta la vida tradicional, no Health de GAS: agotar solo GAS no convierte al enemigo en cadáver.
bool AKrerusEnemy::IsCorpse() const
{
    if (!IsValid(AttributeComponent))
    {
        return false;
    }

    return !AttributeComponent->IsAlive();
}

// Retira la selección del controlador local 0 y oculta el cadáver sin destruir el actor.
void AKrerusEnemy::OnCorpseTimeExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Ha terminado el tiempo del cadaver de %s"), *GetName());

    // Limpiar la selección si este enemigo está seleccionado.
    if (AKrerusPlayerController* PlayerController =
            Cast<AKrerusPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        PlayerController->ClearSelectionIfTarget(this);
    }

    // Ocultar al enemigo.
    SetActorHiddenInGame(true);

    // Desactivar todas sus colisiones.
    SetActorEnableCollision(false);
}

UAbilitySystemComponent* AKrerusEnemy::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}