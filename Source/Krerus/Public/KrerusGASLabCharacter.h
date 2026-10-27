#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "KrerusGASLabCharacter.generated.h"

class UKrerusGASLabAttributeSet;
class UGameplayAbility;

/**
 * Personaje del laboratorio GAS: aloja su propio ASC y concede una habilidad inicial.
 * Es un circuito de prueba separado del Character jugable y de su PlayerState.
 */
UCLASS()
class KRERUS_API AKrerusGASLabCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusGASLabCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    void ActivateLabAbility();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UKrerusGASLabAttributeSet> AttributeSet;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASLab|Abilities")
    TSubclassOf<UGameplayAbility> StartupAbilityClass;

public:
    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
