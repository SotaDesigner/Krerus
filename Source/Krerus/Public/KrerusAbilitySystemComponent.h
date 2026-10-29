// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeInitialization.h"
#include "KrerusAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FKrerusASCDeathDelegate)


UCLASS()
class KRERUS_API UKrerusAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	/**
 * Inicializa las estadísticas GAS una única vez.
 *
 * Debe llamarse en el servidor después de configurar
 * el ActorInfo y disponer del AttributeSet.
 */
	bool InitializeAttributes(const FKrerusAttributeInitialization& Config);

	UKrerusAbilitySystemComponent();
	FKrerusASCDeathDelegate OnASCDeath;

protected:
	virtual void BeginPlay() override;
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleDeath();

private:

	bool bAttributeInitialized = false;
	bool bInitialStatsApplied = false;

};
