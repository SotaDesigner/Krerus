#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusResourceTypes.h"
#include "KrerusAttributeTypes.h"
#include "GameplayEffectTypes.h"
#include "KrerusAttributeComponent.generated.h"
class UAbilitySystemComponent;
class UKrerusAttributeSet;
/** Operaciones Blueprint sobre GAS: sin almacén de atributos ni suscripciones propias. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KRERUS_API UKrerusAttributeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKrerusAttributeComponent();
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

protected:
    // Solo selecciona el recurso visible; InitialAttributes del actor configura GAS.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes|Resources") FKrerusResource Resource;
private:
    UAbilitySystemComponent* GetASC() const;
    const UKrerusAttributeSet* GetAttributes() const;
    bool ApplyDelta(FName Attribute, float Amount);
    FName ResourceName() const;
    TMap<FGuid, FActiveGameplayEffectHandle> ModifierHandles;
};
