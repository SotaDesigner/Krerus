#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KrerusPlayerController.generated.h"

class AKrerusEnemy;

UCLASS()
class KRERUS_API AKrerusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void UpdateMouseCapture(bool bIsCameraControlActive, bool bIsFreeCameraActive);

	// Compatibilidad con mapas que todavia usen el PlayerController anterior.
	static void ApplyMouseCapture(APlayerController* PC, bool bIsCameraControlActive, bool bIsFreeCameraActive);

	void SelectEnemyAtScreenPosition(const FVector2D& ScreenPosition);
	void BeginLeftMouseClick();
	void EndLeftMouseClick();
	void RegisterLeftMouseMovement(const FVector2D& MouseDelta);
	void SetSelectedEnemy(AKrerusEnemy* NewEnemy);
	void ClearSelectedEnemy();

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Targeting")
	TWeakObjectPtr<AKrerusEnemy> SelectedEnemy;

	FVector2D LeftClickScreenPosition = FVector2D::ZeroVector;
	bool bLeftMousePressed = false;
	bool bLeftMouseDragged = false;

	float LeftMouseDragDistance = 0.0f;

	float LeftMouseDragThreshold = 5.0f;
};
