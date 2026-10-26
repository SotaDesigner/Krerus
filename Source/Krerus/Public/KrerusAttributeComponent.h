#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusResourceTypes.h"
#include "KrerusAttributeTypes.h"
#include "KrerusAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged,float, CurrentHealth,float, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResourceChanged,EKrerusResourceType, ResourceType,float, CurrentValue,float, MaxValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributesChanged,FKrerusPrimaryAttributes, NewAttributes);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDerivedAttributesChanged,FKrerusDerivedAttributes, NewAttributes);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRERUS_API UKrerusAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UKrerusAttributeComponent();

	UFUNCTION(BlueprintPure,Category = "Attributes|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure,Category = "Attributes|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Attributes|Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Health")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintPure, Category = "Attributes|Health")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Attributes|Resource")
	EKrerusResourceType GetResourceType() const	{return Resource.Type;}

	UFUNCTION(BlueprintPure, Category = "Attributes|Resource")
	float GetCurrentResource() const {return Resource.CurrentValue;}

	UFUNCTION(BlueprintPure, Category = "Attributes|Resource")
	float GetMaxResource() const {return Resource.MaxValue;}

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributesChanged OnAttributesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnDerivedAttributesChanged OnDerivedAttributesChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Attributes|Health", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,Category = "Attributes|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Attributes|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Attributes|Resurces")
	FKrerusResource Resource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Attributes|Primary")
	FKrerusPrimaryAttributes BaseAttributes;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,Category = "Attributes|Primary")
	FKrerusPrimaryAttributes FinalAttributes;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,Category = "Attributes|Primary")
	FKrerusAttributeModifier AttributeBonuses;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,Category = "Attributes|Primary")
	TArray<FKrerusAttributeModifier> ActiveModifiers;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Derived")
	FKrerusDerivedAttributes DerivedAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Attributes|Scaling")
	FKrerusAttributeScaling AttributeScaling;

	// Called when the game starts
	virtual void BeginPlay() override;
	void InitializeResource();
	void RecalculateAttributes();
	void RecalculateDerivedAttributes();
	void RecalculateMaxHealth();

	UFUNCTION(BlueprintCallable, Category = "Attributes|Modifiers")
	FGuid AddAttributeModifier(FKrerusAttributeModifier Modifier);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Modifiers")
	bool RemoveAttributeModifier(FGuid ModifierId);

	UFUNCTION(BlueprintPure, Category = "Attributes|Primary")
	FKrerusPrimaryAttributes GetFinalAttributes() const	{return FinalAttributes;}

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnResourceChanged OnResourceChanged;

	UFUNCTION(BlueprintCallable,Category = "Attributes|Resource")
	bool TryConsumeResource(float Amount);

	UFUNCTION(BlueprintCallable,Category = "Attributes|Resource")
	void RestoreResource(float Amount);

	UFUNCTION(BlueprintPure, Category = "Attributes|Derived")
	FKrerusDerivedAttributes GetDerivedAttributes() const {return DerivedAttributes;}
};
