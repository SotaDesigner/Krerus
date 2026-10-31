
#include "KrerusStrikeAbility.h"
#include "KrerusAbilityMovementComponent.h"
#include "KrerusTargetPositioningLibrary.h"
#include "KrerusCharacter.h"
#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusCombatStateComponent.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UKrerusStrikeAbility::UKrerusStrikeAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UKrerusStrikeAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    auto Fail = [&]()
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        };

    if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
    {
        Fail();
        return;
    }

    AKrerusCharacter* Character =
        Cast<AKrerusCharacter>(ActorInfo->AvatarActor.Get());

    if (!IsValid(Character) || !Character->HasAuthority())
    {
        Fail();
        return;
    }

    AKrerusPlayerController* PC =
        Cast<AKrerusPlayerController>(Character->GetController());

    AKrerusEnemy* Target = Character->GetStrikeTarget();

    if (!IsValid(PC) || !IsValid(Target))
    {
        Fail();
        return;
    }

    UAbilitySystemComponent* SourceASC =
        Character->GetAbilitySystemComponent();

    const UKrerusAttributeSet* PlayerAttributes =
        IsValid(SourceASC)
        ? SourceASC->GetSet<UKrerusAttributeSet>()
        : nullptr;

    UAbilitySystemComponent* TargetASC =
        Target->GetAbilitySystemComponent();

    const UKrerusAttributeSet* TargetAttributes =
        IsValid(TargetASC)
        ? TargetASC->GetSet<UKrerusAttributeSet>()
        : nullptr;

    if (!PlayerAttributes ||
        PlayerAttributes->GetHealth() <= 0.f ||
        !TargetAttributes ||
        TargetAttributes->GetHealth() <= 0.f ||
        Target->IsCorpse() ||
        Target->GetWorld() != Character->GetWorld())
    {
        Fail();
        return;
    }

    const float Distance = FVector::Dist(
        Character->GetActorLocation(),
        Target->GetActorLocation());

    if (!FMath::IsFinite(Distance) ||
        Distance > AbilityRange)
    {
        Fail();
        return;
    }

    // Una animación con impacto necesita un evento válido.
    if (AttackMontage && !ImpactEventTag.IsValid())
    {
        Fail();
        return;
    }

    // Si configuramos daño, necesitamos un GameplayEffect.
    if (BaseAbilityDamage > 0.f && !DamageEffectClass)
    {
        Fail();
        return;
    }

    FVector MovementDestination = FVector::ZeroVector;

    if (MovementType != EKrerusAbilityMovementType::None)
    {
        if (MovementType != EKrerusAbilityMovementType::Teleport)
        {
            Fail();
            return;
        }

        if (!UKrerusTargetPositioningLibrary::
            FindValidPositionAroundTarget(
                Character,
                Target,
                PositionType,
                MovementDistance,
                MovementDestination))
        {
            Fail();
            return;
        }
    }

    // Validamos coste y cooldown antes de ejecutar efectos.
    if (!CheckCost(Handle, ActorInfo) ||
        !CheckCooldown(Handle, ActorInfo))
    {
        Fail();
        return;
    }

    // Ejecutar movimiento opcional.
    if (MovementType == EKrerusAbilityMovementType::Teleport)
    {
        UKrerusAbilityMovementComponent* Movement =
            Character->GetAbilityMovementComponent();

        if (!IsValid(Movement) ||
            !Movement->TeleportToPosition(
                MovementDestination,
                Target))
        {
            Fail();
            return;
        }
    }

    // Coste y cooldown solo después del movimiento válido.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        Fail();
        return;
    }

    PendingImpactTarget = Target;
    bImpactApplied = false;

    // Sin animación: daño inmediato, como antes.
    if (!AttackMontage)
    {
        ApplyAbilityImpact(Target);
        FinishStrikeAbility(false);
        return;
    }

    // Escuchar impacto ANTES de iniciar la animación.
    UAbilityTask_WaitGameplayEvent* ImpactTask =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
            this,
            ImpactEventTag,
            nullptr,
            true,
            true);

    if (!IsValid(ImpactTask))
    {
        FinishStrikeAbility(true);
        return;
    }

    ImpactTask->EventReceived.AddDynamic(
        this,
        &UKrerusStrikeAbility::OnAbilityImpact);

    ImpactTask->ReadyForActivation();

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            1.0f);

    if (!IsValid(MontageTask))
    {
        FinishStrikeAbility(true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(
        this,
        &UKrerusStrikeAbility::OnAbilityMontageCompleted);

    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UKrerusStrikeAbility::OnAbilityMontageInterrupted);

    MontageTask->OnCancelled.AddDynamic(
        this,
        &UKrerusStrikeAbility::OnAbilityMontageCancelled);

    MontageTask->ReadyForActivation();

    // La habilidad sigue activa esperando impacto o fin del montaje.
}

void UKrerusStrikeAbility::OnAbilityImpact(
    FGameplayEventData Payload)
{
    if (!IsActive() || bImpactApplied)
    {
        return;
    }

    if (Payload.EventTag != ImpactEventTag)
    {
        return;
    }

    ApplyAbilityImpact(PendingImpactTarget);
}

void UKrerusStrikeAbility::OnAbilityMontageCompleted()
{
    FinishStrikeAbility(false);
}

void UKrerusStrikeAbility::OnAbilityMontageInterrupted()
{
    FinishStrikeAbility(true);
}

void UKrerusStrikeAbility::OnAbilityMontageCancelled()
{
    FinishStrikeAbility(true);
}

void UKrerusStrikeAbility::FinishStrikeAbility(
    bool bWasCancelled)
{
    if (!IsActive())
    {
        return;
    }

    EndAbility(
        GetCurrentAbilitySpecHandle(),
        GetCurrentActorInfo(),
        GetCurrentActivationInfo(),
        true,
        bWasCancelled);
}

void UKrerusStrikeAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    PendingImpactTarget = nullptr;
    bImpactApplied = false;

    Super::EndAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateEndAbility,
        bWasCancelled);
}

bool UKrerusStrikeAbility::ApplyAbilityImpact(
    AKrerusEnemy* Target)
{
    if (bImpactApplied || !IsValid(Target) || Target->IsCorpse())
    {
        return false;
    }

    AKrerusCharacter* Character =
        Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());

    UAbilitySystemComponent* SourceASC =
        GetAbilitySystemComponentFromActorInfo();

    UAbilitySystemComponent* TargetASC =
        Target->GetAbilitySystemComponent();

    if (!IsValid(Character) ||
        !Character->HasAuthority() ||
        !IsValid(SourceASC) ||
        !IsValid(TargetASC))
    {
        return false;
    }

    const UKrerusAttributeSet* TargetAttributes =
        TargetASC->GetSet<UKrerusAttributeSet>();

    if (!TargetAttributes ||
        TargetAttributes->GetHealth() <= 0.f)
    {
        return false;
    }

    // Impedimos procesar dos veces el mismo impacto.
    bImpactApplied = true;

    // Daño opcional.
    if (DamageEffectClass && BaseAbilityDamage > 0.f)
    {
        FGameplayEffectContextHandle Context =
            SourceASC->MakeEffectContext();

        Context.AddSourceObject(Character);

        FGameplayEffectSpecHandle DamageSpec =
            SourceASC->MakeOutgoingSpec(
                DamageEffectClass,
                GetAbilityLevel(),
                Context);

        if (!DamageSpec.IsValid())
        {
            return false;
        }

        DamageSpec.Data->SetSetByCallerMagnitude(
            FGameplayTag::RequestGameplayTag(
                FName(TEXT("Data.Damage"))),
            -FMath::Max(0.f, BaseAbilityDamage));

        const FActiveGameplayEffectHandle AppliedHandle =
            TargetASC->ApplyGameplayEffectSpecToSelf(
                *DamageSpec.Data.Get());

        if (!AppliedHandle.WasSuccessfullyApplied())
        {
            return false;
        }
    }

    // Efecto adicional opcional.
    if (AdditionalEffectClass &&
        !Target->IsCorpse() &&
        TargetAttributes->GetHealth() > 0.f)
    {
        FGameplayEffectSpecHandle AdditionalSpec =
            MakeOutgoingGameplayEffectSpec(
                AdditionalEffectClass,
                1.0f);

        if (AdditionalSpec.IsValid())
        {
            TargetASC->ApplyGameplayEffectSpecToSelf(
                *AdditionalSpec.Data.Get());
        }
    }

    // Registrar relación de combate.
    if (!Target->IsCorpse() &&
        TargetAttributes->GetHealth() > 0.f)
    {
        UKrerusCombatStateComponent* CombatState =
            Character->GetCombatStateComponent();

        if (IsValid(CombatState))
        {
            CombatState->AddCombatRelation(Target);
        }
    }

    UE_LOG(LogTemp, Display,
        TEXT("KRERUS GAS: Impacto aplicado a %s"),
        *GetNameSafe(Target));

    return true;
}
