#include "KrerusCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

void UKrerusCameraComponent::InitializeCamera(USpringArmComponent* CameraBoom)
{
	if (CameraBoom)
	{
		TargetCameraDistance = CameraBoom->TargetArmLength;
	}
}

void UKrerusCameraComponent::UpdateCamera(USpringArmComponent* CameraBoom, float DeltaTime, float CameraZoomInterpSpeed)
{
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetCameraDistance, DeltaTime, CameraZoomInterpSpeed);
	}
}

void UKrerusCameraComponent::ZoomCamera(float ZoomInput, float CameraZoomStep, float MinCameraDistance, float MaxCameraDistance)
{
	TargetCameraDistance = FMath::Clamp(TargetCameraDistance - ZoomInput * CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

void UKrerusCameraComponent::SetupCameraLimits(APlayerController* PC)
{
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->ViewPitchMin = -75.0f;
		PC->PlayerCameraManager->ViewPitchMax = 45.0f;
	}
}
