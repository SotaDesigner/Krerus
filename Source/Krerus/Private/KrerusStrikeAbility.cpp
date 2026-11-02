
#include "KrerusStrikeAbility.h"
#include "KrerusAbilityMovementComponent.h"
#include "KrerusTargetPositioningLibrary.h"
#include "KrerusCharacter.h"
#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusCombatStateComponent.h"
#include "KrerusNPC.h"
#include "KrerusSpellField.h"

#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "AbilitySystemBlueprintLibrary.h"
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

    AKrerusCharacter* Character = Cast<AKrerusCharacter>(ActorInfo->AvatarActor.Get());

    if (!IsValid(Character) || !Character->HasAuthority())
    {
        Fail();
        return;
    }

    if (bIsFieldAbility)
    {
        if (bIsHealingAbility)
        {
            FinishStrikeAbility(true);
            return;
        }

        BeginFieldAbility();
        return;
    }

    if (bIsHealingAbility)
    {
        BeginHealingAbility();
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

bool UKrerusStrikeAbility::IsValidHealingTarget(AActor* Target) const
{
    AActor* Avatar = GetAvatarActorFromActorInfo();

    if (!IsValid(Avatar) || !Avatar->HasAuthority() || !IsValid(Target) || Target->GetWorld() != Avatar->GetWorld())
    {
        return false;
    }

    if (Target != Avatar)
    {
        const AKrerusNPC* NPC = Cast<AKrerusNPC>(Target);
        if (!NPC || Cast<AKrerusEnemy>(Target) || !NPC->CanReceivePlayerHealing())
        {
            return false;
        }
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

    const UKrerusAttributeSet* TargetAttributes = IsValid(TargetASC) ? TargetASC->GetSet<UKrerusAttributeSet>() : nullptr;

    if (!TargetAttributes || TargetAttributes->GetHealth() <= 0.0f)
    {
        return false;
    }

    if (!FMath::IsFinite(AbilityRange) || AbilityRange < 0.0f)
    {
        return false;
    }

    const float Distance = FVector::Dist(Avatar->GetActorLocation(), Target->GetActorLocation());

    return FMath::IsFinite(Distance) && Distance <= AbilityRange;
}

void UKrerusStrikeAbility::BeginHealingAbility()
{
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

    const UKrerusAttributeSet* Attributes = IsValid(SourceASC) ? SourceASC->GetSet<UKrerusAttributeSet>() : nullptr;

    if (!HealingEffectClass || !Attributes || Attributes->GetHealth() <= 0.0f || !FMath::IsFinite(HealingCastTime) || HealingCastTime < 0.0f)
    {
        FinishStrikeAbility(true);
        return;
    }

    AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());

    if (!IsValid(Character))
    {
        FinishStrikeAbility(true);
        return;
    }

    AActor* Target = Character->GetAbilityTarget();

    if (!IsValid(Target))
    {
        Target = Character;
    }

    if (!IsValidHealingTarget(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS HEAL: Objetivo no valido"));

        FinishStrikeAbility(true);
        return;
    }

    PendingHealingTarget = Target;

    if (!CheckCost(GetCurrentAbilitySpecHandle(),GetCurrentActorInfo()) || !CheckCooldown(GetCurrentAbilitySpecHandle(),GetCurrentActorInfo()))
    {
        FinishStrikeAbility(true);
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("KRERUS HEAL: Inicio de lanzamiento | Duracion=%.2f"), HealingCastTime);

    if (HealingCastTime <= 0.0f)
    {
        OnHealingCastCompleted();
        return;
    }

    UAbilityTask_WaitDelay* CastTask = UAbilityTask_WaitDelay::WaitDelay(this, HealingCastTime);

    if (!IsValid(CastTask))
    {
        FinishStrikeAbility(true);
        return;
    }

    CastTask->OnFinish.AddDynamic(this, &UKrerusStrikeAbility::OnHealingCastCompleted);

    CastTask->ReadyForActivation();
}

void UKrerusStrikeAbility::OnHealingCastCompleted()
{
    if (!IsActive())
    {
        return;
    }

    AActor* Avatar = GetAvatarActorFromActorInfo();
    AActor* Target = PendingHealingTarget.Get();

    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

    const UKrerusAttributeSet* SourceAttributes = IsValid(SourceASC) ? SourceASC->GetSet<UKrerusAttributeSet>() : nullptr;

    if (!IsValid(Avatar) || !Avatar->HasAuthority() || !SourceAttributes || SourceAttributes->GetHealth() <= 0.0f || !HealingEffectClass || !IsValidHealingTarget(Target))
    {
        FinishStrikeAbility(true);
        return;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

    const UKrerusAttributeSet* TargetAttributes = IsValid(TargetASC) ? TargetASC->GetSet<UKrerusAttributeSet>() : nullptr;

    if (!TargetAttributes)
    {
        FinishStrikeAbility(true);
        return;
    }

    const float HealingAmount = BaseHealing + SpellPowerHealingCoefficient * SourceAttributes->GetSpellPower();

    if (!FMath::IsFinite(HealingAmount) || HealingAmount <= 0.0f)
    {
        FinishStrikeAbility(true);
        return;
    }

    const FGameplayTag HealingTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Healing")), false);

    if (!HealingTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("KRERUS HEAL: Falta registrar Data.Healing"));

        FinishStrikeAbility(true);
        return;
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();

    Context.AddSourceObject(Avatar);

    FGameplayEffectSpecHandle HealingSpec = SourceASC->MakeOutgoingSpec(HealingEffectClass, GetAbilityLevel(), Context);

    if (!HealingSpec.IsValid())
    {
        FinishStrikeAbility(true);
        return;
    }

    HealingSpec.Data->SetSetByCallerMagnitude(HealingTag, HealingAmount);

    if (!CommitAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo()))
    {
        FinishStrikeAbility(true);
        return;
    }

    const float HealthBefore = TargetAttributes->GetHealth();

    const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*HealingSpec.Data.Get());

    if (!AppliedHandle.WasSuccessfullyApplied())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS HEAL: No se pudo aplicar la curacion"));

        FinishStrikeAbility(true);
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("KRERUS HEAL: Curacion=%.1f | HP %.1f -> %.1f / %.1f | Mana=%.1f"),
        HealingAmount, HealthBefore, TargetAttributes->GetHealth(), TargetAttributes->GetMaxHealth(), SourceAttributes->GetMana());

    FinishStrikeAbility(false);
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
    PendingHealingTarget = nullptr;

    Super::EndAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateEndAbility,
        bWasCancelled);
}

bool UKrerusStrikeAbility::ApplyAbilityImpact(AKrerusEnemy* Target)
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

void UKrerusStrikeAbility::BeginFieldAbility()
{
    AKrerusCharacter* Character = Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    const UKrerusAttributeSet* Attributes = IsValid(ASC) ? ASC->GetSet<UKrerusAttributeSet>() : nullptr;

    UWorld* World = GetWorld();

    if (!IsValid(Character) || !Character->HasAuthority() || !World || !Attributes || Attributes->GetHealth() <= 0.f ||
        !FieldClass || !FMath::IsFinite(AbilityRange) || AbilityRange < 0.f)
    {
        FinishStrikeAbility(true);
        return;
    }

    FVector Location;
    FRotator Rotation(0.f, Character->GetActorRotation().Yaw, 0.f);

    if (FieldPlacement == EKrerusFieldPlacement::CursorGround)
    {
        FVector RequestedLocation;

        if (!Character->GetAbilityGroundLocation(RequestedLocation) || RequestedLocation.ContainsNaN())
        {
            FinishStrikeAbility(true);
            return;
        }

        if (FVector::DistSquared(Character->GetActorLocation(), RequestedLocation) > FMath::Square(AbilityRange))
        {
            FinishStrikeAbility(true);
            return;
        }

        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Character);

        FHitResult GroundHit;

        const bool bFoundGround = World->LineTraceSingleByChannel(GroundHit, RequestedLocation + FVector(0.f, 0.f, 100.f),
            RequestedLocation - FVector(0.f,0.f,100.f), ECC_GameTraceChannel1, Params);

        if (!bFoundGround || GroundHit.ImpactNormal.Z < 0.7f || FVector::DistSquared(GroundHit.ImpactPoint, RequestedLocation) > FMath::Square(30.f))
        {
            FinishStrikeAbility(true);
            return;
        }

        Location = GroundHit.ImpactPoint;

        if (FVector::DistSquared(Character->GetActorLocation(), Location) > FMath::Square(AbilityRange))
        {
            FinishStrikeAbility(true);
            return;
        }

        FHitResult ObstancleHit;

        const FVector Start = Character->GetActorLocation();

        const FVector End = Location + FVector(0.f, 0.f, 30.f);

        if (World->LineTraceSingleByChannel(ObstancleHit, Start, End, ECC_Visibility, Params))
        {
            FinishStrikeAbility(true);
            return;
        }
    }
    else
    {
        Location = Character->GetActorLocation();
    }

    const auto Handle = GetCurrentAbilitySpecHandle();
    const auto* Info = GetCurrentActorInfo();
    const auto Activation = GetCurrentActivationInfo();

    if (!CheckCost(Handle, Info) || !CheckCooldown(Handle, Info))
    {
        FinishStrikeAbility(true);
        return;
    }

    const FTransform Transform(Rotation, Location);

    AKrerusSpellField* Field = World->SpawnActorDeferred<AKrerusSpellField>(FieldClass, Transform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Field)
    {
        FinishStrikeAbility(true);
        return;
    }

    if (!CommitAbility(Handle, Info, Activation))
    {
        Field->Destroy();
        FinishStrikeAbility(true);
        return;
    }

    Field->FinishSpawning(Transform);

    const bool bInitialized = IsValid(Field) && Field->InitializeField(Character, GetAbilityLevel());

    if (!bInitialized && IsValid(Field))
    {
        Field->Destroy();
    }

    FinishStrikeAbility(!bInitialized);
}