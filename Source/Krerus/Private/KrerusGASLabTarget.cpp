#include "KrerusGASLabTarget.h"
#include "AbilitySystemComponent.h"
#include "KrerusGASLabAttributeSet.h"
#include "Components/SceneComponent.h"

AKrerusGASLabTarget::AKrerusGASLabTarget()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    RootComponent = SceneRoot;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    AbilitySystemComponent->SetIsReplicated(true);

    AttributeSet = CreateDefaultSubobject<UKrerusGASLabAttributeSet>(TEXT("AttributeSet"));
}

void AKrerusGASLabTarget::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }

    if (AbilitySystemComponent->HasAttributeSetForAttribute(UKrerusGASLabAttributeSet::GetHealthAttribute()))
    {
        const float Health =
            AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetHealthAttribute());

        const float MaxHealth =
            AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetMaxHealthAttribute());

        const float Mana = AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetManaAttribute());

        const float MaxMana =
            AbilitySystemComponent->GetNumericAttribute(UKrerusGASLabAttributeSet::GetMaxManaAttribute());

        UE_LOG(LogTemp, Display, TEXT("[GASLabTarget] %s | Health: %.1f / %.1f | Mana: %.1f / %.1f"), *GetName(),
            Health, MaxHealth, Mana, MaxMana);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GASLabTarget] El ASC no reconoce el AttributeSet"));
    }
}

void AKrerusGASLabTarget::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

UAbilitySystemComponent* AKrerusGASLabTarget::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}
