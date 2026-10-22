#include "KrerusPlayerController.h"

void AKrerusPlayerController::UpdateMouseCapture(bool bIsCameraControlActive, bool bIsFreeCameraActive)
{
	ApplyMouseCapture(this, bIsCameraControlActive, bIsFreeCameraActive);
}

void AKrerusPlayerController::ApplyMouseCapture(APlayerController* PC, bool bIsCameraControlActive, bool bIsFreeCameraActive)
{
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (bIsCameraControlActive || bIsFreeCameraActive)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}
