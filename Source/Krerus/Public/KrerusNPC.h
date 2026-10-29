#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KrerusInteractable.h"
#include "AbilitySystemInterface.h"
#include "KrerusNPC.generated.h"

class UKrerusAttributeComponent;
class UAbilitySystemComponent;
class UKrerusAttributeSet;

/**
 * Base seleccionable e interactuable de los NPC y enemigos.
 * Expone los atributos GAS y delega la representación del anillo de selección en Blueprint.
 */
UCLASS()
class KRERUS_API AKrerusNPC : public ACharacter, public IKrerusInteractable, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusNPC();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UKrerusAttributeSet> GASAttributeSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interacion", meta = (ClampMin = "0.0"))
    float InteractionDistance = 300.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    TObjectPtr<UKrerusAttributeComponent> AttributeComponent;

public:
    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Interact_Implementation(AActor* Interactor) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Targeting")
    void SetSelectionRingVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category = "Attributes")
    UKrerusAttributeComponent* GetAttributeComponent() const
    {
        return AttributeComponent.Get();
    }

    float GetInteractionDistance() const
    {
        return InteractionDistance;
    }
};
