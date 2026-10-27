#include "KrerusStrikeAbility.h"

#include "KrerusCharacter.h"
#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UKrerusStrikeAbility::UKrerusStrikeAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// Valida Avatar, controlador, enemigo seleccionado y distancia 3D antes de confirmar coste/enfriamiento.
// El daño proviene de DamageEffectClass y se aplica al ASC del enemigo; termina la habilidad en cada salida.
void UKrerusStrikeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !DamageEffectClass)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AKrerusCharacter* Character = Cast<AKrerusCharacter>(ActorInfo->AvatarActor.Get());

    if (!IsValid(Character))
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

    AKrerusEnemy* Target = Cast<AKrerusEnemy>(PC->GetSelectedNPC());

    if (!IsValid(Target) || !IsValid(Target->GetAbilitySystemComponent()))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const float Distance = FVector::Dist(Character->GetActorLocation(), Target->GetActorLocation());

    if (Distance > AbilityRange)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // El coste se aplica una sola vez.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, 1.0f);

    if (DamageSpec.IsValid())
    {
        Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

        UE_LOG(LogTemp, Display, TEXT("KRERUS GAS: Strike aplicado a %s"), *GetNameSafe(Target));

        const UKrerusAttributeSet* TargetAttributes =
            Target->GetAbilitySystemComponent()->GetSet<UKrerusAttributeSet>();

        const UKrerusAttributeSet* PlayerAttributes = ActorInfo->AbilitySystemComponent->GetSet<UKrerusAttributeSet>();

        if (TargetAttributes && PlayerAttributes)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("KRERUS GAS: RESULTADO | Enemigo HP: %.1f / %.1f | Jugador Mana: %.1f / %.1f"),
                TargetAttributes->GetHealth(), TargetAttributes->GetMaxHealth(), PlayerAttributes->GetMana(),
                PlayerAttributes->GetMaxMana());
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}