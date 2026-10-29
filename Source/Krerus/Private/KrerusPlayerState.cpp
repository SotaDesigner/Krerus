#include "KrerusPlayerState.h"
#include "KrerusAutoAttackAbility.h"
#include "KrerusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

AKrerusPlayerState::AKrerusPlayerState()
{
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    AbilitySystemComponent->SetIsReplicated(true);

    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<UKrerusAttributeSet>(TEXT("AttributeSet"));

    SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AKrerusPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UKrerusAttributeSet* AKrerusPlayerState::GetKrerusAttributeSet() const
{
    return AttributeSet;
}

// Concede StartupAbilityClass únicamente con autoridad; el Character inicializa después su vínculo como Avatar.
void AKrerusPlayerState::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority() && AbilitySystemComponent) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UKrerusAutoAttackAbility::StaticClass(),1));

    if (HasAuthority() && StartupAbilityClass && AbilitySystemComponent)
    {
        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbilityClass, 1, 0));
    }
}