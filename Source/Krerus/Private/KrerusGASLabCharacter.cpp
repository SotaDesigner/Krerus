#include "KrerusGASLabCharacter.h"
#include "AbilitySystemComponent.h"
#include "KrerusGASLabAttributeSet.h"
#include "KrerusGASLabStrike.h"
#include "Abilities/GameplayAbility.h"
#include "InputCoreTypes.h"
#include "Components/InputComponent.h"

AKrerusGASLabCharacter::AKrerusGASLabCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    AbilitySystemComponent->SetIsReplicated(true);

    AttributeSet = CreateDefaultSubobject<UKrerusGASLabAttributeSet>(TEXT("AttributeSet"));

    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
}

UAbilitySystemComponent* AKrerusGASLabCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AKrerusGASLabCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        UE_LOG(LogTemp, Display, TEXT("[GASLab] ASC inicializado. Owner: %s | Avatar: %s"),
            *GetNameSafe(AbilitySystemComponent->GetOwnerActor()),
            *GetNameSafe(AbilitySystemComponent->GetAvatarActor()));
    }

    const float Health = AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetHealthAttribute());

    const float MaxHealth =
        AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetMaxHealthAttribute());

    const float Mana = AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetManaAttribute());

    const float MaxMana = AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetMaxManaAttribute());

    UE_LOG(
        LogTemp, Display, TEXT("[GASLab] Health: %.1f / %.1f | Mana: %.1f / %.1f"), Health, MaxHealth, Mana, MaxMana);

    if (HasAuthority() && StartupAbilityClass)
    {
        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbilityClass, 1, 0));
    }
}

void AKrerusGASLabCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AKrerusGASLabCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AKrerusGASLabCharacter::ActivateLabAbility);
}

// La prueba solicita la habilidad inicial solo con autoridad y no consulta el targeting del juego.
void AKrerusGASLabCharacter::ActivateLabAbility()
{
    if (!AbilitySystemComponent || !StartupAbilityClass)
    {
        return;
    }

    if (!HasAuthority())
    {
        return;
    }

    const bool bActivated = AbilitySystemComponent->TryActivateAbilityByClass(StartupAbilityClass);

    UE_LOG(
        LogTemp, Display, TEXT("[GASLab] Activacion solicitada: %s"), bActivated ? TEXT("Correcta") : TEXT("Fallida"));
}
