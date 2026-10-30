
#include "KrerusStrikeAbility.h"

#include "KrerusCharacter.h"
#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusCombatStateComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UKrerusStrikeAbility::UKrerusStrikeAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// Valida Avatar, controlador, objetivo, vida GAS y distancia antes de consumir recursos.
// El daño se aplica al ASC del enemigo mediante DamageEffectClass.
void UKrerusStrikeAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !DamageEffectClass)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AKrerusCharacter* Character = Cast<AKrerusCharacter>(ActorInfo->AvatarActor.Get());

    if (!IsValid(Character) || !Character->HasAuthority())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(Character->GetController());

    if (!IsValid(PC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Objetivo explícito recibido por el RPC del Character, nunca selección local del servidor.
    AKrerusEnemy* Target = Character->GetStrikeTarget();

    if (!IsValid(Target))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const auto* PlayerASC=Character->GetAbilitySystemComponent();
    const auto* PlayerGAS=PlayerASC?PlayerASC->GetSet<UKrerusAttributeSet>():nullptr;
    if (!PlayerGAS || PlayerGAS->GetHealth()<=0.f ||
        Target->GetWorld() != Character->GetWorld())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    // Obtenemos el ASC del enemigo para consultar su vida real.
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    if (!IsValid(TargetASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const UKrerusAttributeSet* TargetAttributes =
        TargetASC->GetSet<UKrerusAttributeSet>();

    // Un enemigo sin vida GAS no puede recibir Strike ni provocar gasto de mana.
    if (!IsValid(TargetAttributes) || TargetAttributes->GetHealth() <= 0.0f)
    {
        UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: Strike cancelado. Objetivo sin vida."));

        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const float Distance = FVector::Dist(Character->GetActorLocation(),Target->GetActorLocation());

    if (!FMath::IsFinite(Distance) || Distance > AbilityRange || Target->IsCorpse())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Solo consumimos recursos cuando el objetivo es valido, esta vivo y en alcance.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!IsValid(SourceASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    Context.AddSourceObject(Character);
    FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
        DamageEffectClass, GetAbilityLevel(), Context);

    if (DamageSpec.IsValid())
    {
        DamageSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage"))),-FMath::Max(0.0f, BaseAbilityDamage));
    }

    if (DamageSpec.IsValid())
    {
        const float HealthBefore = TargetAttributes->GetHealth();
        const float ArmorBefore = TargetAttributes->GetArmor();

        UE_LOG(LogTemp, Verbose,
            TEXT("KRERUS STRIKE DEBUG: DamageEffect=%s | RawDamage=%.1f | HP antes=%.1f | Armor=%.1f"),
            *GetNameSafe(DamageEffectClass.Get()),
            BaseAbilityDamage,
            HealthBefore,
            ArmorBefore);

        const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

        const float HealthAfter = TargetAttributes->GetHealth();

        UE_LOG(LogTemp, Verbose,
            TEXT("KRERUS STRIKE DEBUG: Aplicado=%s | HP despues=%.1f | Daño real=%.1f"),
            AppliedHandle.WasSuccessfullyApplied() ? TEXT("SI") : TEXT("NO"),
            HealthAfter,
            HealthBefore - HealthAfter);

        if (!AppliedHandle.WasSuccessfullyApplied())
        {
            UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: Strike no pudo aplicar su GameplayEffect"));

            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        // Aplicar el efecto adicional si está configurado
        // y el objetivo sigue vivo tras recibir el daño.
        if (AdditionalEffectClass && IsValid(Target) && !Target->IsCorpse() && TargetAttributes->GetHealth() > 0.0f)
        {
            FGameplayEffectSpecHandle AdditionalSpec = MakeOutgoingGameplayEffectSpec(AdditionalEffectClass, 1.0f);

            if (AdditionalSpec.IsValid())
            {
                const FActiveGameplayEffectHandle AdditionalHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*AdditionalSpec.Data.Get());

                if (AdditionalHandle.WasSuccessfullyApplied())
                {
                    UE_LOG(LogTemp, Verbose, TEXT("KRERUS GAS: Efecto adicional aplicado a %s"), *GetNameSafe(Target));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("KRERUS GAS: No se pudo aplicar el efecto adicional a %s"), *GetNameSafe(Target));                
                }
            }
        }

        // Registrar la relación si el enemigo sigue vivo.
        if (!Target->IsCorpse() && TargetAttributes->GetHealth() > 0.0f)
        {
            UKrerusCombatStateComponent* CombatState =
                Character->GetCombatStateComponent();

            if (IsValid(CombatState))
            {
                CombatState->AddCombatRelation(Target);
            }
        }

        UE_LOG(LogTemp,Display,TEXT("KRERUS GAS: Strike aplicado a %s"),*GetNameSafe(Target));

        const UKrerusAttributeSet* PlayerAttributes = ActorInfo->AbilitySystemComponent->GetSet<UKrerusAttributeSet>();

        if (PlayerAttributes)
        {
            UE_LOG(LogTemp,Warning,TEXT("KRERUS GAS: RESULTADO | Enemigo HP: %.1f / %.1f | Jugador Mana: %.1f / %.1f"),TargetAttributes->GetHealth(),
                TargetAttributes->GetMaxHealth(),PlayerAttributes->GetMana(),PlayerAttributes->GetMaxMana());
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
