#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "KrerusGASLabTarget.generated.h"

class UAbilitySystemComponent;
class UKrerusGASLabAttributeSet;
class USceneComponent;

/**
 * Objetivo del laboratorio con ASC y UKrerusGASLabAttributeSet propios.
 * UKrerusGASLabStrike lo busca por clase, sin utilizar la selección del PlayerController jugable.
 */
UCLASS()
class KRERUS_API AKrerusGASLabTarget : public AActor, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusGASLabTarget();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UKrerusGASLabAttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};
