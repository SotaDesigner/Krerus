#include "KrerusNPC.h"
#include "KrerusAttributeComponent.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.h"

AKrerusNPC::AKrerusNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
    GASAttributeSet = CreateDefaultSubobject<UKrerusAttributeSet>(TEXT("GASAttributeSet"));
    AttributeComponent = CreateDefaultSubobject<UKrerusAttributeComponent>(TEXT("AttributeComponent"));
}

void AKrerusNPC::BeginPlay()
{
    Super::BeginPlay();
    AbilitySystemComponent->InitAbilityActorInfo(this,this);
    if (AttributeComponent) AttributeComponent->InitializeGAS();
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

UAbilitySystemComponent* AKrerusNPC::GetAbilitySystemComponent() const { return AbilitySystemComponent; }
