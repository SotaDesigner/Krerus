#include "KrerusCombatComponent.h"
#include "KrerusEnemy.h"
#include "TimerManager.h"
#include "KrerusAttributeComponent.h"
#include "KrerusCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "KrerusAttributeSet.h"
#include "Net/UnrealNetwork.h"

UKrerusCombatComponent::UKrerusCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UKrerusCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner());

    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

    if (!IsValid(PlayerAttributes))
    {
        return;
    }

    PlayerAttributes->OnDeath.AddUniqueDynamic(this, &UKrerusCombatComponent::OnOwnerDeath);
}

void UKrerusCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAutoAttack();

    if (AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner()))
    {
        if (UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent())
        {
            PlayerAttributes->OnDeath.RemoveDynamic(this, &UKrerusCombatComponent::OnOwnerDeath);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UKrerusCombatComponent::SetCombatTarget(AKrerusEnemy* NewTarget)
{
    if (!GetOwner()->HasAuthority())
    {
        // Selección local; el objetivo explícito se enviará al iniciar.
        if (CombatTarget.Get() != NewTarget) StopAutoAttack();
        CombatTarget = NewTarget;
        return;
    }
    // Si es el mismo objetivo, no hacemos nada.
    if (CombatTarget.Get() == NewTarget)
    {
        return;
    }

    // Detenemos el combate anterior antes de cambiar de objetivo.
    StopAutoAttack();

    // Guardamos el nuevo objetivo.
    CombatTarget = NewTarget;
}

AKrerusEnemy* UKrerusCombatComponent::GetCombatTarget() const
{
    return CombatTarget.Get();
}

// El servidor valida al atacante y la vida GAS del objetivo y programa la cadencia.
bool UKrerusCombatComponent::StartAutoAttack()
{
    if (!GetOwner()->HasAuthority())
    {
        if (AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetOwner()))
        {
            if (!IsValid(CombatTarget.Get()) || CombatTarget->IsCorpse()) return false;
            Character->ServerStartAutoAttack(CombatTarget.Get());
            return true; // Solicitud enviada; OnRep confirma el estado, no predice el montaje.
        }
        return false;
    }
    if (!IsValid(CombatTarget.Get()))
    {
        return false;
    }

    if (bIsAutoAttacking)
    {
        return false;
    }

    AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner());

    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

    if (!IsValid(PlayerAttributes) || !PlayerAttributes->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS: No se puede iniciar el autoataque. El jugador esta muerto."));

        return false;
    }


    if (!IsValid(CombatTarget.Get()) || CombatTarget->IsCorpse())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: No se puede atacar. Objetivo invalido o muerto."));

        return false;
    }

    // La selección recibida del cliente debe superar las reglas del servidor.
    if (CombatTarget->GetWorld() != GetWorld() || !IsTargetInRange(PlayerCharacter, CombatTarget.Get()) ||
        !IsTargetInFront(PlayerCharacter, CombatTarget.Get())) return false;

    bIsAutoAttacking = true;

    GetWorld()->GetTimerManager().SetTimer(
        AutoAttackTimerHandle, this, &UKrerusCombatComponent::PerformAutoAttack, GetAttackInterval(), true);
    OnAutoAttackStarted.Broadcast();

    return true;
}

// Cancela el temporizador e invalida cualquier impacto pendiente en autoridad.
void UKrerusCombatComponent::StopAutoAttack()
{
    if (!GetOwner()->HasAuthority())
    {
        if (!GetOwner()->IsActorBeingDestroyed())
            if (AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetOwner())) Character->ServerStopAutoAttack();
        return;
    }
    const bool bWasAutoAttacking = bIsAutoAttacking;


    bIsAutoAttacking = false;
    bAttackImpactConsumed = true;
    bAutoAttackPaused = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
    }

    if (bWasAutoAttacking)
    {
        if (AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetOwner()))
            Character->MulticastAutoAttackMontage(false, GetAttackInterval());
        OnAutoAttackStopped.Broadcast();
    }
}

bool UKrerusCombatComponent::IsAutoAttacking() const
{
    return bIsAutoAttacking;
}

float UKrerusCombatComponent::GetAttackInterval() const
{
    return FMath::Max(0.1f, AttackInterval);
}

// Consume como máximo un impacto por montaje y vuelve a validar alcance, arco frontal y vida.
// AttackPower tradicional alimenta la fórmula; únicamente el GE modifica Health GAS.

void UKrerusCombatComponent::ApplyAutoAttackImpact()
{
    if (!GetOwner()->HasAuthority()) return;
    AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner());

    if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsAutoAttackMontagePlaying())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Impacto descartado. Montage de ataque inactivo."));
        return;
    }

    if (bAttackImpactConsumed)
    {
        return;
    }

    bAttackImpactConsumed = true;

    if (!bIsAutoAttacking || bAutoAttackPaused)
    {
        return;
    }

    // El daño es autoritativo: solo el servidor modifica la vida GAS.
    if (!PlayerCharacter->HasAuthority())
    {
        return;
    }

    AKrerusEnemy* Target = CombatTarget.Get();

    if (!IsValid(Target))
    {
        StopAutoAttack();
        return;
    }

    // Conservamos el componente tradicional para calcular las estadísticas.
    UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

    if (!IsValid(PlayerAttributes) || !PlayerAttributes->IsAlive())
    {
        StopAutoAttack();
        return;
    }

    // Obtenemos los sistemas GAS del atacante y del enemigo.
    UAbilitySystemComponent* AttackerASC = PlayerCharacter->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    if (!IsValid(AttackerASC) || !IsValid(TargetASC) || !AutoAttackDamageEffectClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS GAS: Autoataque cancelado. ASC o GameplayEffect no valido."));
        return;
    }

    // Comprobamos la vida real del enemigo, gestionada por GAS.
    const UKrerusAttributeSet* TargetAttributeSet =
        TargetASC->GetSet<UKrerusAttributeSet>();

    if (Target->IsCorpse() || !IsValid(TargetAttributeSet) || TargetAttributeSet->GetHealth() <= 0.0f)
    {
        StopAutoAttack();
        return;
    }

    if (!IsTargetInRange(PlayerCharacter, Target))
    {
        return;
    }

    if (!IsTargetInFront(PlayerCharacter, Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Impacto fallido. Objetivo fuera del arco frontal."));
        return;
    }

    // Conservamos la formula de daño original de Krerus.
    const FKrerusDerivedAttributes DerivedAttributes =
        PlayerAttributes->GetDerivedAttributes();

    const float Damage =
        BaseAttackDamage + DerivedAttributes.AttackPower * AttackPowerCoefficient;

    if (Damage <= 0.0f)
    {
        return;
    }

    // Creamos el GameplayEffect desde el ASC del atacante.
    FGameplayEffectContextHandle EffectContext = AttackerASC->MakeEffectContext();
    EffectContext.AddSourceObject(PlayerCharacter);

    FGameplayEffectSpecHandle SpecHandle = AttackerASC->MakeOutgoingSpec(
        AutoAttackDamageEffectClass,
        1.0f,
        EffectContext
    );

    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS GAS: No se pudo crear el efecto de autoataque."));
        return;
    }

    // El modificador SetByCaller necesita una magnitud negativa
    // porque estamos restando directamente al atributo Health.
    const FGameplayTag DamageTag =
        FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage")));

    SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -Damage);

    // Aplicamos el efecto sobre el mismo Health GAS que usa Krerus Strike.
    const FActiveGameplayEffectHandle AppliedHandle =
        AttackerASC->ApplyGameplayEffectSpecToTarget(
            *SpecHandle.Data.Get(),
            TargetASC
        );

    if (!AppliedHandle.WasSuccessfullyApplied())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS GAS: No se pudo aplicar el daño del autoataque."));
        return;
    }

    const float RemainingHealth = TargetAttributeSet->GetHealth();

    UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: Autoataque aplicado a %s | Daño: %.1f | Vida GAS restante: %.1f"),*Target->GetName(),Damage,RemainingHealth);

    OnAutoAttackHit.Broadcast(Target, Damage);

    if (Target->IsCorpse()) StopAutoAttack();
}


void UKrerusCombatComponent::OnCombatTargetDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: El objetivo ha muerto. Deteniendo autoataque."));

    StopAutoAttack();
}

void UKrerusCombatComponent::OnOwnerDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: El jugador ha muerto. Deteniendo autoataque."));

    StopAutoAttack();
}

// El temporizador inicia la animación si alcance, orientación y estado lo permiten; aquí no se aplica daño.
void UKrerusCombatComponent::PerformAutoAttack()
{
    if (!GetOwner()->HasAuthority() || !bIsAutoAttacking) return;
    UE_LOG(LogTemp, Warning, TEXT("KRERUS DEBUG: AutoAttack=%s | Paused=%s | Target=%s"),
        bIsAutoAttacking ? TEXT("TRUE") : TEXT("FALSE"), bAutoAttackPaused ? TEXT("TRUE") : TEXT("FALSE"),
        IsValid(CombatTarget.Get()) ? *CombatTarget->GetName() : TEXT("NONE"));

    if (bAutoAttackPaused || GetWorld()->GetTimeSeconds() < NextAttackTime)
    {
        return;
    }

    AActor* Owner = GetOwner();
    AKrerusEnemy* Target = CombatTarget.Get();

    if (!IsValid(Owner) || !IsValid(Target))
    {
        StopAutoAttack();
        return;
    }

    AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(Owner);

    if (!IsValid(PlayerCharacter))
    {
        StopAutoAttack();
        return;
    }

    UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

    if (!IsValid(PlayerAttributes) || !PlayerAttributes->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS: El jugador esta muerto. Autoataque detenido."));

        StopAutoAttack();
        return;
    }

    if (Target->IsCorpse())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS: Objetivo muerto o sin atributos. Autoataque detenido."));

        StopAutoAttack();
        return;
    }

    if (!IsTargetInRange(PlayerCharacter, Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS: Objetivo fuera de alcance"));

        return;
    }

    if (!IsTargetInFront(PlayerCharacter, Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Objetivo fuera del arco frontal"));

        return;
    }

    if (PlayerCharacter->IsAutoAttackMontagePlaying())
    {
        UE_LOG(LogTemp, Verbose, TEXT("KRERUS COMBAT: Ataque omitido. El Montage anterior sigue activo."));

        return;
    }

    PlayerCharacter->MulticastAutoAttackMontage(true, GetAttackInterval());
    bAttackImpactConsumed = !PlayerCharacter->IsAutoAttackMontagePlaying();
    if (!bAttackImpactConsumed) NextAttackTime = GetWorld()->GetTimeSeconds() + GetAttackInterval();
}

bool UKrerusCombatComponent::IsTargetInRange(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const
{
    if (!IsValid(PlayerCharacter) || !IsValid(Target))
    {
        return false;
    }

    const float Distance = FVector::Dist2D(PlayerCharacter->GetActorLocation(), Target->GetActorLocation());

    return Distance <= AttackRange;
}

bool UKrerusCombatComponent::IsTargetInFront(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const
{
    if (!IsValid(PlayerCharacter) || !IsValid(Target))
    {
        return false;
    }

    const FVector DirectionToTarget =
        (Target->GetActorLocation() - PlayerCharacter->GetActorLocation()).GetSafeNormal2D();

    const FVector ForwardDirection = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();

    const float FacingDot = FVector::DotProduct(ForwardDirection, DirectionToTarget);

    return FacingDot >= 0.0f;
}

void UKrerusCombatComponent::CancelPendingAutoAttackImpact()
{
    bAttackImpactConsumed = true;

    UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Impacto pendiente cancelado."));
}

// Al reanudar intenta un ataque inmediato y restablece el temporizador si el combate sigue activo.
void UKrerusCombatComponent::SetAutoAttackPaused(bool bPaused)
{
    if (!GetOwner()->HasAuthority())
    {
        if (AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetOwner())) Character->ServerSetAutoAttackPaused(bPaused);
        return;
    }
    // Evitar procesar dos veces el mismo estado.
    if (bAutoAttackPaused == bPaused)
    {
        return;
    }

    bAutoAttackPaused = bPaused;
    if (bPaused)
    {
        CancelPendingAutoAttackImpact();
        if (AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetOwner()))
            Character->MulticastAutoAttackMontage(false, GetAttackInterval());
    }

    UE_LOG(LogTemp, Log, TEXT("KRERUS COMBAT: Autoataque %s"), bAutoAttackPaused ? TEXT("PAUSADO") : TEXT("REANUDADO"));

    if (!bAutoAttackPaused && bIsAutoAttacking)
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);

        PerformAutoAttack();

        if (bIsAutoAttacking)
        {
            GetWorld()->GetTimerManager().SetTimer(
                AutoAttackTimerHandle, this, &UKrerusCombatComponent::PerformAutoAttack, GetAttackInterval(), true);
        }
    }
}

bool UKrerusCombatComponent::IsAutoAttackPaused() const
{
    return bAutoAttackPaused;
}
void UKrerusCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent, CombatTarget, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent, bIsAutoAttacking, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent, bAutoAttackPaused, COND_OwnerOnly);
}

void UKrerusCombatComponent::OnRep_AutoAttacking()
{
    if (bIsAutoAttacking) OnAutoAttackStarted.Broadcast();
    else OnAutoAttackStopped.Broadcast();
}
