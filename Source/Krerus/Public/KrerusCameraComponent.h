#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCameraComponent.generated.h"

class USpringArmComponent;
class APlayerController;

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
