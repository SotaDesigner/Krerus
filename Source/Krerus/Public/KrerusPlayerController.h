#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KrerusPlayerController.generated.h"

UCLASS()
class KRERUS_API AKrerusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void UpdateMouseCapture(bool bIsCameraControlActive, bool bIsFreeCameraActive);

	// Compatibilidad con mapas que todavia usen el PlayerController anterior.
	static void ApplyMouseCapture(APlayerController* PC, bool bIsCameraControlActive, bool bIsFreeCameraActive);
};
