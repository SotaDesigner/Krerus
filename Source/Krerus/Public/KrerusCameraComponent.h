#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCameraComponent.generated.h"

class USpringArmComponent;
class APlayerController;

/**
 * Mantiene la distancia deseada del zoom y los límites verticales de cámara.
 * El Character aporta su SpringArm y llama a UpdateCamera desde Tick; el componente no decide la selección.
 */
UCLASS()
class KRERUS_API UKrerusCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void InitializeCamera(USpringArmComponent* CameraBoom);
    void UpdateCamera(USpringArmComponent* CameraBoom, float DeltaTime, float CameraZoomInterpSpeed);
    void ZoomCamera(float ZoomInput, float CameraZoomStep, float MinCameraDistance, float MaxCameraDistance);
    void SetupCameraLimits(APlayerController* PC);

protected:
    float TargetCameraDistance = 1400.0f;
};
