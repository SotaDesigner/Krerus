#include "KrerusPlayerState.h"
#include "KrerusAutoAttackAbility.h"
#include "KrerusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "KrerusAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

AKrerusPlayerState::AKrerusPlayerState()
{
    AbilitySystemComponent = CreateDefaultSubobject<UKrerusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

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
}

void AKrerusPlayerState::InitializeCharacterClass(EKrerusClass NewClass)
{
    if (!HasAuthority() || bCharacterClassInitiaized)
    {
        return;
    }
    bCharacterClassInitiaized = true;
    CharacterClass = NewClass;
    
    GrantStartingAbilities();

    UE_LOG(LogTemp, Log, TEXT("[Krerus Class] Clase inicializada: %s"), *UEnum::GetValueAsString(CharacterClass));
}

void AKrerusPlayerState::OnRep_CharacterClass()
{

}

void AKrerusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AKrerusPlayerState, CharacterClass);
}

const UKrerusClassDataAsset* AKrerusPlayerState::GetClassDefinition() const
{
    const TObjectPtr<UKrerusClassDataAsset>* Definition =
        ClassDefinitions.Find(CharacterClass);

    return Definition ? Definition->Get() : nullptr;
}

void AKrerusPlayerState::GrantStartingAbilities()
{
    if (!HasAuthority() || !AbilitySystemComponent)
    {
        return;
    }

    const UKrerusClassDataAsset* Definition = GetClassDefinition();

    if (!Definition)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Krerus Class] No existe Data Asset para la clase %s"),
            *UEnum::GetValueAsString(CharacterClass));
        return;
    }

    int32 AbilitySlot = 1;

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : Definition->StartingAbilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        // El autoataque no ocupa una ranura numerada.
        const bool bIsAutoAttack = AbilityClass->IsChildOf(UKrerusAutoAttackAbility::StaticClass());

        const int32 InputID = bIsAutoAttack ? INDEX_NONE : AbilitySlot++;

        bool bAlreadyGranted = false;

        for (const FGameplayAbilitySpec& Spec :
            AbilitySystemComponent->GetActivatableAbilities())
        {
            if (Spec.Ability &&
                Spec.Ability->GetClass() == AbilityClass.Get())
            {
                bAlreadyGranted = true;
                break;
            }
        }

        if (!bAlreadyGranted)
        {
            AbilitySystemComponent->GiveAbility(
                FGameplayAbilitySpec(AbilityClass, 1, InputID));

            UE_LOG(LogTemp, Log,
                TEXT("[Krerus GAS] Habilidad %s concedida | Ranura: %d"),
                *GetNameSafe(AbilityClass.Get()),
                InputID);
        }
    }
}