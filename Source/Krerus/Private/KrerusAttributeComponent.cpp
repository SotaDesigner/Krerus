#include "KrerusAttributeComponent.h"

UKrerusAttributeComponent::UKrerusAttributeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

// Modifica la vida tradicional y emite los eventos que usan las reacciones, la muerte y el autoataque.
void UKrerusAttributeComponent::ApplyDamage(float DamageAmount)
{
    if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || !IsAlive() || CurrentHealth <= 0.0f)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

    if (PreviousHealth != CurrentHealth)
    {
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

        UE_LOG(LogTemp, Warning, TEXT("KRERUS: Daño recibido | Vida: %.1f / %.1f"), CurrentHealth, MaxHealth);
    }

    if (PreviousHealth >= 0.0f && CurrentHealth <= 0.0f)
    {
        OnDeath.Broadcast();

        UE_LOG(LogTemp, Warning, TEXT("KRERUS: %s ha muerto"), *GetNameSafe(GetOwner()));
    }
}

void UKrerusAttributeComponent::Heal(float HealAmount)
{
    if (!FMath::IsFinite(HealAmount) || HealAmount <= 0.0f || !IsAlive() || CurrentHealth <= 0.0f)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;

    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);

    if (CurrentHealth != PreviousHealth)
    {
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

        UE_LOG(LogTemp, Warning, TEXT("KRERUS: Curación recibida | Vida: %.1f / %.1f"), CurrentHealth, MaxHealth);
    }
}

void UKrerusAttributeComponent::BeginPlay()
{
    Super::BeginPlay();

    BaseMaxHealth = FMath::Max(BaseMaxHealth, 1.0f);

    RecalculateAttributes();

    CurrentHealth = MaxHealth;

    InitializeResource();
}

// Maná y energía comienzan llenos; ira y None comienzan vacíos dentro del sistema tradicional.
void UKrerusAttributeComponent::InitializeResource()
{
    Resource.MaxValue = FMath::Max(Resource.MaxValue, 0.0f);

    switch (Resource.Type)
    {
    case EKrerusResourceType::Mana:
    case EKrerusResourceType::Energy:
        Resource.CurrentValue = Resource.MaxValue;
        break;

    case EKrerusResourceType::Rage:
    case EKrerusResourceType::None:
    default:
        Resource.CurrentValue = 0.0f;
        break;
    }
}

// Agrega modificadores, recalcula atributos primarios y derivados y ajusta la vida máxima antes de notificar.
void UKrerusAttributeComponent::RecalculateAttributes()
{
    const FKrerusPrimaryAttributes PreviousAttributes = FinalAttributes;

    AttributeBonuses = FKrerusAttributeModifier();

    for (const FKrerusAttributeModifier& Modifier : ActiveModifiers)
    {
        AttributeBonuses.Strength += Modifier.Strength;
        AttributeBonuses.Agility += Modifier.Agility;
        AttributeBonuses.Intellect += Modifier.Intellect;
        AttributeBonuses.Stamina += Modifier.Stamina;

        AttributeBonuses.AttackPower += Modifier.AttackPower;
        AttributeBonuses.SpellPower += Modifier.SpellPower;
        AttributeBonuses.Armor += Modifier.Armor;
        AttributeBonuses.CriticalChance += Modifier.CriticalChance;
    }

    FinalAttributes.Strength = FMath::Max(0.0f, BaseAttributes.Strength + AttributeBonuses.Strength);

    FinalAttributes.Agility = FMath::Max(0.0f, BaseAttributes.Agility + AttributeBonuses.Agility);

    FinalAttributes.Intellect = FMath::Max(0.0f, BaseAttributes.Intellect + AttributeBonuses.Intellect);

    FinalAttributes.Stamina = FMath::Max(0.0f, BaseAttributes.Stamina + AttributeBonuses.Stamina);

    RecalculateDerivedAttributes();

    RecalculateMaxHealth();

    const bool bAttributesChanged = PreviousAttributes.Strength != FinalAttributes.Strength ||
        PreviousAttributes.Agility != FinalAttributes.Agility ||
        PreviousAttributes.Intellect != FinalAttributes.Intellect ||
        PreviousAttributes.Stamina != FinalAttributes.Stamina;

    if (bAttributesChanged)
    {
        OnAttributesChanged.Broadcast(FinalAttributes);
    }
}

void UKrerusAttributeComponent::RecalculateDerivedAttributes()
{
    const FKrerusDerivedAttributes PreviousAttributes = DerivedAttributes;

    DerivedAttributes.AttackPower = FMath::Max(0.0f,
        FinalAttributes.Strength * AttributeScaling.StrengthToAttackPower +
            FinalAttributes.Agility * AttributeScaling.AgilityToAttackPower + AttributeBonuses.AttackPower);

    DerivedAttributes.SpellPower = FMath::Max(
        0.0f, FinalAttributes.Intellect * AttributeScaling.IntellectToSpellPower + AttributeBonuses.SpellPower);

    DerivedAttributes.Armor =
        FMath::Max(0.0f, FinalAttributes.Stamina * AttributeScaling.StaminaToArmor + AttributeBonuses.Armor);

    DerivedAttributes.CriticalChance = FMath::Clamp(
        FinalAttributes.Agility * AttributeScaling.AgilityToCriticalChance + AttributeBonuses.CriticalChance, 0.0f,
        100.0f);

    const bool bDerivedAttributesChanged = PreviousAttributes.AttackPower != DerivedAttributes.AttackPower ||
        PreviousAttributes.SpellPower != DerivedAttributes.SpellPower ||
        PreviousAttributes.Armor != DerivedAttributes.Armor ||
        PreviousAttributes.CriticalChance != DerivedAttributes.CriticalChance;

    if (bDerivedAttributesChanged)
    {
        OnDerivedAttributesChanged.Broadcast(DerivedAttributes);
    }
}

void UKrerusAttributeComponent::RecalculateMaxHealth()
{
    const float PreviousMaxHealth = MaxHealth;
    const float PreviousCurrentHealth = CurrentHealth;

    const float NewMaxHealth =
        FMath::Max(1.0f, BaseMaxHealth + FinalAttributes.Stamina * AttributeScaling.StaminaToHealth);

    MaxHealth = NewMaxHealth;

    // Nunca curamos automáticamente al aumentar la vida máxima.
    // Si disminuye, limitamos la vida actual al nuevo máximo.
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

    if (PreviousMaxHealth != MaxHealth || PreviousCurrentHealth != CurrentHealth)
    {
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    }
}

FGuid UKrerusAttributeComponent::AddAttributeModifier(FKrerusAttributeModifier Modifier)
{
    Modifier.Id = FGuid::NewGuid();

    ActiveModifiers.Add(Modifier);

    RecalculateAttributes();

    return Modifier.Id;
}

bool UKrerusAttributeComponent::RemoveAttributeModifier(FGuid ModifierId)
{
    if (!ModifierId.IsValid())
    {
        return false;
    }

    for (int32 Index = 0; Index < ActiveModifiers.Num(); ++Index)
    {
        if (ActiveModifiers[Index].Id == ModifierId)
        {
            ActiveModifiers.RemoveAt(Index);

            RecalculateAttributes();

            return true;
        }
    }

    return false;
}

void UKrerusAttributeComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Consume el recurso tradicional; los costes de habilidades GAS se resuelven por su propio circuito.
bool UKrerusAttributeComponent::TryConsumeResource(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return false;
    }

    if (Resource.Type == EKrerusResourceType::None)
    {
        return false;
    }

    if (Resource.CurrentValue < Amount)
    {
        return false;
    }

    Resource.CurrentValue = FMath::Clamp(Resource.CurrentValue - Amount, 0.0f, Resource.MaxValue);
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: Recurso consumido: %.1f | Restante: %.1f / %.1f"), Amount,
        Resource.CurrentValue, Resource.MaxValue);

    OnResourceChanged.Broadcast(Resource.Type, Resource.CurrentValue, Resource.MaxValue);

    return true;
}

void UKrerusAttributeComponent::RestoreResource(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return;
    }

    const float PreviousValue = Resource.CurrentValue;

    if (Resource.Type == EKrerusResourceType::None)
    {
        return;
    }

    Resource.CurrentValue = FMath::Clamp(Resource.CurrentValue + Amount, 0.0f, Resource.MaxValue);
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: Recurso recuperado: %.1f | Actual: %.1f / %.1f"), Amount,
        Resource.CurrentValue, Resource.MaxValue);

    if (PreviousValue != Resource.CurrentValue)
    {
        OnResourceChanged.Broadcast(Resource.Type, Resource.CurrentValue, Resource.MaxValue);
    }
}

bool UKrerusAttributeComponent::IsAlive() const
{
    return CurrentHealth > 0.0f;
}
