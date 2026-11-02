// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "KrerusSpellField.generated.h"

class UBoxComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class AKrerusCharacter;
class AKrerusEnemy;

UENUM(BlueprintType)
enum class EKrerusFieldShape : uint8
{
	Circle,
	Cone
};

UENUM(BlueprintType)
enum class EKrerusFieldApplication : uint8
{
	Once,
	Periodic
};

UCLASS()
class KRERUS_API AKrerusSpellField : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKrerusSpellField();

	bool InitializeField(AKrerusCharacter* InCaster, float InAbilityLevel);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> DetectionBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	EKrerusFieldShape Shape = EKrerusFieldShape::Circle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	EKrerusFieldApplication Application = EKrerusFieldApplication::Periodic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field", meta = (ClampMin = "1.0"))
	float Radius = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field", meta = (ClampMin = "1.0"))
	float HalfHeight = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float ConeAngleDegrees = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field", meta = (ClampMin = "1.0"))
	float FieldDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field", meta = (ClampMin = "0.1"))
	float PulseInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	bool bDestroyWhenCasterDies = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Effects")
	TSubclassOf<UGameplayEffect> PulseEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Effects")
	TSubclassOf<UGameplayEffect> WhileInsideEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Effects", meta = (ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Effects", meta = (ClampMin = "0.0"))
	float SpellPowerCoefficient = 0.2f;

public:

private:

	UPROPERTY(Transient)
	TObjectPtr<AKrerusCharacter> Caster;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	TSet<TWeakObjectPtr<AKrerusEnemy>> Candidates;

	TMap<TWeakObjectPtr<AKrerusEnemy>, FActiveGameplayEffectHandle> InsideEffects;

	FTimerHandle UpdateTimer;
	
	float AbilityLevel = 1.f;
	float SnapshotDamage = 0.f;
	double StartTime = 0.0;
	double NextPulseTime = 0.0;
	bool bInitialized = false;

	UFUNCTION()
	void OnDetectionBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool IsValidTarget(AKrerusEnemy* Enemy) const;
	bool IsInsideShape(AKrerusEnemy* Enemy) const;

	void UpdateField();
	void RefreshInsideEffects();
	void ApplyPulse();
	void RemoveInsideEffect(AKrerusEnemy* Enemy);
	FActiveGameplayEffectHandle ApplyEffect(AKrerusEnemy* Enemy, TSubclassOf<UGameplayEffect> EffectClass, bool bSetDamage);
	
};
