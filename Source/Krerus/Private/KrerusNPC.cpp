#include "KrerusNPC.h"
#include "KrerusAttributeComponent.h"

AKrerusNPC::AKrerusNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    AttributeComponent = CreateDefaultSubobject<UKrerusAttributeComponent>(TEXT("AttributeComponent"));
}

void AKrerusNPC::BeginPlay()
{
    Super::BeginPlay();
}

void AKrerusNPC::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AKrerusNPC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AKrerusNPC::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: Interaccion con NPC: %s"), *GetName());
}
