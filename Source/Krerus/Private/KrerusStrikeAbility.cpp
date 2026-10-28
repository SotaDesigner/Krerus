
#include "KrerusStrikeAbility.h"

#include "KrerusCharacter.h"
#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusAttributeComponent.h"

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

    if (!Character->GetAttributeComponent() || !Character->GetAttributeComponent()->IsAlive() ||
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("KRERUS GAS: Strike cancelado. Objetivo sin vida.")
        );

        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const float Distance = FVector::Dist(
        Character->GetActorLocation(),
        Target->GetActorLocation()
    );

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

    FGameplayEffectSpecHandle DamageSpec =
        MakeOutgoingGameplayEffectSpec(DamageEffectClass, 1.0f);

    if (DamageSpec.IsValid())
    {
        TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

        UE_LOG(
            LogTemp,
            Display,
            TEXT("KRERUS GAS: Strike aplicado a %s"),
            *GetNameSafe(Target)
        );

        const UKrerusAttributeSet* PlayerAttributes =
            ActorInfo->AbilitySystemComponent->GetSet<UKrerusAttributeSet>();

        if (PlayerAttributes)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("KRERUS GAS: RESULTADO | Enemigo HP: %.1f / %.1f | Jugador Mana: %.1f / %.1f"),
                TargetAttributes->GetHealth(),
                TargetAttributes->GetMaxHealth(),
                PlayerAttributes->GetMana(),
                PlayerAttributes->GetMaxMana()
            );
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
