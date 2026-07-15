#include "Components/InteractableComponent.h"

// Sets default values for this component's properties
UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UInteractableComponent::Interact(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}

	OnInteract.Broadcast(Interactor);
	return true;
}

bool UInteractableComponent::CanInteract_Implementation(AActor* Interactor) const
{
	return bInteractionEnabled && IsValid(Interactor) && IsValid(GetOwner());
}

void UInteractableComponent::SetInteractionEnabled(bool bEnabled)
{
	bInteractionEnabled = bEnabled;
}


// Called when the game starts
void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}