// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KrerusProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class KRERUS_API AKrerusProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKrerusProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Component")
	TObjectPtr<USphereComponent> Collider;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	float ProjectileDamage = 0.0f;
	TWeakObjectPtr<AActor> Target = nullptr;
	UPROPERTY(Transient)
    TSubclassOf<UGameplayEffect> DamageEffectClass;
    UPROPERTY(Transient)
    TSubclassOf<UGameplayEffect> ResourceEffectClass;
    TWeakObjectPtr<UAbilitySystemComponent> ASC;
    float SourceAbilityLevel = 1.f;
    bool bHitResolved = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void InitializeProjectile(AActor* target, float damage, UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffectClass,
        TSubclassOf<UGameplayEffect> InResourceEffectClass = nullptr, float InAbilityLevel = 1.f);
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,	FVector NormalImpulse, const FHitResult& Hit);
};
