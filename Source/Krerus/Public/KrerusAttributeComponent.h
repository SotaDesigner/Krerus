#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusResourceTypes.h"
#include "KrerusAttributeTypes.h"
#include "GameplayEffectTypes.h"
#include "KrerusAttributeComponent.generated.h"
class UAbilitySystemComponent;
class UKrerusAttributeSet;
struct FOnAttributeChangeData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnResourceChanged, EKrerusResourceType, ResourceType, float, CurrentValue, float, MaxValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributesChanged, FKrerusPrimaryAttributes, NewAttributes);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDerivedAttributesChanged, FKrerusDerivedAttributes, NewAttributes);


/** Blueprint-compatible view of GAS. No independent health, resources, stats or modifier calculations. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KRERUS_API UKrerusAttributeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKrerusAttributeComponent();
    void InitializeGAS();
    UFUNCTION(BlueprintPure, Category="Attributes|Health") float GetCurrentHealth() const;
    UFUNCTION(BlueprintPure, Category="Attributes|Health") float GetMaxHealth() const;
    UFUNCTION(BlueprintCallable, Category="Attributes|Health") void ApplyDamage(float Amount);
    UFUNCTION(BlueprintCallable, Category="Attributes|Health") void Heal(float Amount);
    UFUNCTION(BlueprintPure, Category="Attributes|Health") bool IsAlive() const;
    UFUNCTION(BlueprintPure, Category="Attributes|Resource") EKrerusResourceType GetResourceType() const { return Resource.Type; }
    UFUNCTION(BlueprintPure, Category="Attributes|Resource") float GetCurrentResource() const;
    UFUNCTION(BlueprintPure, Category="Attributes|Resource") float GetMaxResource() const;
    UFUNCTION(BlueprintCallable, Category="Attributes|Resource") bool TryConsumeResource(float Amount);
    UFUNCTION(BlueprintCallable, Category="Attributes|Resource") void RestoreResource(float Amount);
    UFUNCTION(BlueprintPure, Category="Attributes|Primary") FKrerusPrimaryAttributes GetFinalAttributes() const;
    UFUNCTION(BlueprintPure, Category="Attributes|Derived") FKrerusDerivedAttributes GetDerivedAttributes() const;
    UFUNCTION(BlueprintCallable, Category="Attributes|Modifiers") FGuid AddAttributeModifier(FKrerusAttributeModifier Modifier);
    UFUNCTION(BlueprintCallable, Category="Attributes|Modifiers") bool RemoveAttributeModifier(FGuid Id);

    UPROPERTY(BlueprintAssignable, Category="Attributes|Events") FOnHealthChanged OnHealthChanged;
    UPROPERTY(BlueprintAssignable, Category="Attributes|Events") FOnDeath OnDeath;
    UPROPERTY(BlueprintAssignable, Category="Attributes|Events") FOnResourceChanged OnResourceChanged;
    UPROPERTY(BlueprintAssignable, Category="Attributes|Events") FOnAttributesChanged OnAttributesChanged;
    UPROPERTY(BlueprintAssignable, Category="Attributes|Events") FOnDerivedAttributesChanged OnDerivedAttributesChanged;
protected:
    // Existing Blueprint defaults remain usable as startup configuration, not runtime values.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes|Resources") FKrerusResource Resource;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes|Primary") FKrerusPrimaryAttributes BaseAttributes;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes|Scaling") FKrerusAttributeScaling AttributeScaling;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UAbilitySystemComponent* GetASC() const;
    const UKrerusAttributeSet* GetAttributes() const;
    void OnGASChanged(const FOnAttributeChangeData& Data);
    bool ApplyDelta(FName Attribute, float Amount);
    FName ResourceName() const;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    TMap<FGuid, FActiveGameplayEffectHandle> ModifierHandles;
};
