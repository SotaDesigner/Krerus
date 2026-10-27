#include "KrerusGASLabStrike.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "KrerusGASLabTarget.h"
#include "KrerusGASLabAttributeSet.h"

UKrerusGASLabStrike::UKrerusGASLabStrike()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// Prueba de coste y efecto con autoridad; busca un actor de laboratorio en vez del NPC seleccionado.
void UKrerusGASLabStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!HasAuthority(&ActivationInfo) || !ActorInfo || !ActorInfo->AvatarActor.IsValid() || !DamageEffectClass)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Avatar = ActorInfo->AvatarActor.Get();

    // Selección provisional: buscar un objetivo en el laboratorio.
    AKrerusGASLabTarget* Target =
        Cast<AKrerusGASLabTarget>(UGameplayStatics::GetActorOfClass(Avatar, AKrerusGASLabTarget::StaticClass()));

    if (Target && Target->GetAbilitySystemComponent())
    {
        FGameplayEffectContextHandle Context = MakeEffectContext(Handle, ActorInfo);

        FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, 1.f);

        if (Spec.IsValid())
        {
            Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    const float TargetHealth =
        Target->GetAbilitySystemComponent()->GetNumericAttribute(UKrerusGASLabAttributeSet::GetHealthAttribute());

    const float CasterMana =
        ActorInfo->AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetManaAttribute());

    UE_LOG(LogTemp, Display, TEXT("[GASLab] Ataque ejecutado | Objetivo HP: %.1f | Lanzador MP: %.1f"), TargetHealth,
        CasterMana);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
