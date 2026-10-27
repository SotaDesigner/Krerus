#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KrerusInteractable.generated.h"

// Contrato de interacción C++/Blueprint invocado por el Character sobre el NPC seleccionado.
UINTERFACE(MinimalAPI, Blueprintable)
class UKrerusInteractable : public UInterface
{
    GENERATED_BODY()
};

class KRERUS_API IKrerusInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void Interact(AActor* Interactor);
};
