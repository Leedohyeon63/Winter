// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractableComponent.h"

// Sets default values for this component's properties
UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UInteractableComponent::Interact(AActor* Interactor)
{
	OnInteract.Broadcast(Interactor);
}

bool UInteractableComponent::CanInteract(AActor* Interactor) const
{
	return true;
}


// Called when the game starts
void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}
