#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "KrerusCharacter.generated.h"

class AKrerusEnemy;
class UInputMappingContext;
class UInputAction;
class UKrerusAttributeComponent;
class UKrerusCombatComponent;
class UAbilitySystemComponent;
class AKrerusPlayerState;
struct FInputActionValue;

/**
 * Avatar jugable: coordina entrada, movimiento, cámara y autoataque.
 * El ASC pertenece a AKrerusPlayerState; AttributeComponent conserva los atributos del combate tradicional.
 */
UCLASS()
class KRERUS_API AKrerusCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusCharacter();

    // RPC sobre el Pawn poseído: el cliente transmite intención y objetivo, nunca daño.
    UFUNCTION(Server, Reliable)
    void ServerStartAutoAttack(AKrerusEnemy* Target);
    UFUNCTION(Server, Reliable)
    void ServerStopAutoAttack();
    UFUNCTION(Server, Reliable)
    void ServerSetAutoAttackPaused(bool bPaused);
    UFUNCTION(Server, Reliable)
    void ServerActivateStrike(AKrerusEnemy* Target);
    UFUNCTION(NetMulticast, Reliable)
    void MulticastAutoAttackMontage(bool bPlay, float Interval);

    AKrerusEnemy* GetStrikeTarget() const { return StrikeTarget.Get(); }

private:
    // Solo existe durante la activación síncrona de Strike en el servidor.
    UPROPERTY(Transient)
    TObjectPtr<AKrerusEnemy> StrikeTarget;
public:
    virtual void PawnClientRestart() override;

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    virtual void PossessedBy(AController* NewController) override;

    virtual void OnRep_PlayerState() override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<class USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<class UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CameraControlAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CameraFreeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CameraZoomAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ClearTargetAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Combat")
    TObjectPtr<UInputAction> AutoAttackAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    float CameraFollowSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    float CameraFollowDelay = 0.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
    float MinCameraDistance = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
    float MaxCameraDistance = 1800.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
    float CameraZoomStep = 150.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
    float CameraZoomInterpSpeed = 8.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<class UKrerusCameraComponent> CameraSystem;

    void ZoomCamera(const FInputActionValue& Value);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float WalkSpeed = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    bool bIsDecelerating = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    bool bIsCameraControlActive = false;
    bool bIsFreeCameraActive = false;

    FRotator FreeMovementRotation = FRotator::ZeroRotator;
    bool bWasMoving = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    TObjectPtr<UKrerusAttributeComponent> AttributeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UKrerusCombatComponent> CombatComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> AutoAttackMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|GAS")
    TObjectPtr<UInputAction> StrikeAction;

    UFUNCTION()
    void OnAutoAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void UpdateSprint(float DeltaTime);
    void StartCameraControl();
    void StopCameraControl();
    void StopMovementInput();
    void StartFreeCamera();
    void StopFreeCamera();
    void UpdateMouseCapture();
    void ClearTarget();
    void Interact();
    void OnAutoAttack();
    void InitializeAbilitySystem();
    void ActivateStrike();

public:
    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category = "Combat")
    UKrerusCombatComponent* GetCombatComponent() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void PlayAutoAttackMontage(float AttackInterval);

    bool IsAutoAttackMontagePlaying() const;

    UFUNCTION(BlueprintPure, Category = "Attributes")
    UKrerusAttributeComponent* GetAttributeComponent() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void OnAutoAttackImpact();
};
