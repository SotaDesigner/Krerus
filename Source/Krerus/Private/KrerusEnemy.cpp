#include "KrerusEnemy.h"
#include "KrerusAttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "KrerusPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"

AKrerusEnemy::AKrerusEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    AbilitySystemComponent->SetIsReplicated(true);

    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

    GASAttributeSet = CreateDefaultSubobject<UKrerusAttributeSet>(TEXT("GASAttributeSet"));
}

void AKrerusEnemy::OnGASHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!HasAuthority() || bIsDead) return;
    const float OldHealth = Data.OldValue;
    const float NewHealth = Data.NewValue;

    UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: Enemigo %s | Health: %.1f -> %.1f"),*GetNameSafe(this),OldHealth,NewHealth);

    // Ignorar curaciones y cambios que no reduzcan la vida.
    if (NewHealth >= OldHealth || OldHealth <= 0.0f)
    {
        return;
    }

    // La muerte tiene prioridad sobre cualquier reacción de impacto.
    if (NewHealth <= 0.0f)
    {
        OnEnemyDeath();
        return;
    }

    MulticastHitReaction();
}

// Reacciones y muerte proceden exclusivamente de Health GAS.
void AKrerusEnemy::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(AbilitySystemComponent) || !IsValid(GASAttributeSet))
    {
        UE_LOG(LogTemp, Error, TEXT("KRERUS GAS: Enemy sin ASC o AttributeSet"));
        return;
    }

    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    // Suscribimos las reacciones a Health de GAS, no a la vida tradicional.
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).AddUObject(this, &AKrerusEnemy::OnGASHealthChanged);

    PreviousHealth = GASAttributeSet->GetHealth();
    if (HasAuthority() && PreviousHealth <= 0.0f) OnEnemyDeath();

    UE_LOG(LogTemp,Display,TEXT("KRERUS GAS: Enemy inicializado | Health: %.1f"),PreviousHealth);
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
    if (bIsDead || bCorpseExpired)
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

// El servidor publica la muerte y programa la retirada persistente del cadáver.
void AKrerusEnemy::OnEnemyDeath()
{
    // La lógica autoritativa de muerte solo se ejecuta en el servidor.
    if (!HasAuthority())
    {
        return;
    }

    if (bIsDead)
    {
        return;
    }

    bIsDead = true;

    UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: Enemigo %s ha muerto"),*GetName());

    ForceNetUpdate();
    ApplyDeathState();
    // Iniciar el temporizador de desaparición del cadáver.
    if (CorpseDuration > 0.0f)
        GetWorldTimerManager().SetTimer(CorpseTimerHandle,this,&AKrerusEnemy::OnCorpseTimeExpired,CorpseDuration,false);
    else OnCorpseTimeExpired();
}

// bIsDead también cubre el orden de llegada de la replicación de Health y muerte.
bool AKrerusEnemy::IsCorpse() const
{
    return bIsDead || !IsValid(GASAttributeSet) || GASAttributeSet->GetHealth() <= 0.0f;
}

// Retira la selección del controlador local 0 y oculta el cadáver sin destruir el actor.
void AKrerusEnemy::OnCorpseTimeExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Ha terminado el tiempo del cadaver de %s"), *GetName());

    if (!HasAuthority()) return;
    bCorpseExpired = true;
    ForceNetUpdate();
    OnRep_CorpseExpired();
}

void AKrerusEnemy::OnRep_CorpseExpired()
{
    if (!bCorpseExpired) return;
    // Limpiar la selección local; el puntero del otro jugador no pertenece a este proceso.
    for (TActorIterator<AKrerusPlayerController> It(GetWorld()); It; ++It)
    {
        if (It->IsLocalController()) It->ClearSelectionIfTarget(this);
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

void AKrerusEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AKrerusEnemy, bIsDead);
    DOREPLIFETIME(AKrerusEnemy, bCorpseExpired);
}

void AKrerusEnemy::OnRep_IsDead()
{
    UE_LOG(
        LogTemp,
        Display,
        TEXT("KRERUS NETWORK: %s | Estado de muerte replicado: %s"),
        *GetNameSafe(this),
        bIsDead ? TEXT("MUERTO") : TEXT("VIVO")
    );

    if (bIsDead)
    {
        ApplyDeathState();
    }
}

void AKrerusEnemy::PlayDeathAnimation()
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

    AnimInstance->Montage_Stop(0.1f);
    AnimInstance->Montage_Play(DeathMontage);
}
void AKrerusEnemy::ApplyDeathState()
{
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    if (bDeathVisualsApplied || bCorpseExpired) return;
    bDeathVisualsApplied = true;
    PlayDeathAnimation();
    // Este evento debe contener presentación; recompensas/lógica solo en autoridad.
    BP_OnEnemyDeath();
}

void AKrerusEnemy::MulticastHitReaction_Implementation()
{
    PlayHitReaction();
}
