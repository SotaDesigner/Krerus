// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusAbilitySystemComponent.h"
#include "KrerusAttributeSet.h"
#include "GameFramework/Actor.h"
#include "KrerusGASEffects.h"
#include "GameplayEffect.h"
#include "KrerusCombatComponent.h"
#include "KrerusCombatStateComponent.h"

UKrerusAbilitySystemComponent::UKrerusAbilitySystemComponent()
{

}

void UKrerusAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).AddUObject(this, &UKrerusAbilitySystemComponent::OnHealthAttributeChanged);
}

void UKrerusAbilitySystemComponent::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (Data.OldValue > 0.f && Data.NewValue <= 0.f)
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			HandleDeath();
		}
		OnASCDeath.Broadcast();
	}
}

bool UKrerusAbilitySystemComponent::InitializeAttributes(const FKrerusAttributeInitialization& Config)
{
	AActor* Owner = GetOwner();

	if(!Owner || !Owner->HasAuthority())
	{ 
		return false;
	}

	if (!GetAvatarActor() || !GetSet<UKrerusAttributeSet>())
	{
		return false;
	}

	if (bAttributeInitialized)
	{
		return true;
	}

	FGameplayEffectSpecHandle Init = MakeOutgoingSpec(UKrerusInitialStatsEffect::StaticClass(), 1.f, MakeEffectContext());
	FGameplayEffectSpecHandle Derived = MakeOutgoingSpec(UKrerusDerivedStatsEffect::StaticClass(), 1.f, MakeEffectContext());

	if (!Init.IsValid() || !Derived.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Krerus GAS] Error creando efecto iniciales."))
		return false;
	}

	// Atributos primarios.
	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Strength")),FMath::Max(0.f, Config.Strength));
	
	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Agility")),FMath::Max(0.f, Config.Agility));
	
	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Intellect")),FMath::Max(0.f, Config.Intellect));

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Stamina")),FMath::Max(0.f, Config.Stamina));

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Armor")), FMath::Max(0.f, Config.BaseArmor));

	// Estadísticas derivadas: valores base.
	Init.Data->SetSetByCallerMagnitude(FName(TEXT("AttackPower")), 0.f);
	
	Init.Data->SetSetByCallerMagnitude(FName(TEXT("SpellPower")), 0.f);

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("CriticalChance")), 0.f);

	// Recursos.
	const float EnergyMax = FMath::Max(0.f, Config.MaxEnergy);
	const float RageMax = FMath::Max(0.f, Config.MaxRage);

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("MaxEnergy")), EnergyMax);

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Energy")), EnergyMax);

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("MaxRage")), RageMax);

	Init.Data->SetSetByCallerMagnitude(FName(TEXT("Rage")), 0.f);

	// Coeficientes de escalado.
	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StrengthToAttackPower")),Config.StrengthToAttackPower);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("AgilityToAttackPower")),Config.AgilityToAttackPower);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("IntellectToSpellPower")),Config.IntellectToSpellPower);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("AgilityToCriticalChance")),Config.AgilityToCriticalChance);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StaminaToArmor")),Config.StaminaToArmor);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StaminaToHealth")),Config.StaminaToHealth);

	Derived.Data->SetSetByCallerMagnitude(FName(TEXT("InitialStamina")),FMath::Max(0.f, Config.Stamina));

	// Reintentar el efecto derivado no debe reiniciar recursos ni estadísticas.
	if (!bInitialStatsApplied)
	{
		if (!ApplyGameplayEffectSpecToSelf(*Init.Data.Get()).WasSuccessfullyApplied())
		{
			return false;
		}
		bInitialStatsApplied = true;
	}

	const FActiveGameplayEffectHandle DerivedHandle = ApplyGameplayEffectSpecToSelf(*Derived.Data.Get());

	if(!DerivedHandle.IsValid() || !GetActiveGameplayEffect(DerivedHandle))
	{ 
		UE_LOG(LogTemp, Error, TEXT("[Krerus GAS] Error aplicando estadísticas derivadas"))
		return false;
	}

	bAttributeInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("[Krerus GAS] Estadísticas inicializadas correctamente."))

	return true;
}

void UKrerusAbilitySystemComponent::HandleDeath()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	AActor* Avatar = GetAvatarActor();

	if (!IsValid(Avatar))
	{
		return;
	}

	if (UKrerusCombatComponent* Combat = Avatar->FindComponentByClass<UKrerusCombatComponent>())
	{
		Combat->StopAutoAttack();
	}

	if (UKrerusCombatStateComponent* CombatState = Avatar->FindComponentByClass<UKrerusCombatStateComponent>())
	{
		CombatState->ClearCombatRelations();
	}

	CancelAllAbilities();
}

