#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "KrerusClassTypes.h"
#include "KrerusClassDataAsset.h"
#include "KrerusPlayerState.generated.h"

class UAbilitySystemComponent;
class UKrerusAttributeSet;
class UGameplayAbility;

/**
 * Propietario del ASC y de UKrerusAttributeSet del jugador.
 * AKrerusCharacter lo utiliza como Owner de GAS y se registra como Avatar al poseerse o recibir PlayerState.
 */
UCLASS()
class KRERUS_API AKrerusPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusPlayerState();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UKrerusAttributeSet* GetKrerusAttributeSet() const;

    UFUNCTION(BlueprintPure, Category = "Krerus|Class")
    EKrerusClass GetCharacterClass() const {return CharacterClass; }

    void InitializeCharacterClass(EKrerusClass NewClass);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    const UKrerusClassDataAsset* GetClassDefinition() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UKrerusAttributeSet> AttributeSet;

    UPROPERTY(ReplicatedUsing = OnRep_CharacterClass, BlueprintReadOnly, Category = "Krerus|Class")
    EKrerusClass CharacterClass = EKrerusClass::Warrior;

    UPROPERTY(EditDefaultsOnly, Category = "Krerus|Classes")
    TMap<EKrerusClass, TObjectPtr<UKrerusClassDataAsset>> ClassDefinitions;

    UFUNCTION()
    void OnRep_CharacterClass();

    UPROPERTY()
    bool bCharacterClassInitiaized = false;

    void GrantStartingAbilities();

    virtual void BeginPlay() override;
};