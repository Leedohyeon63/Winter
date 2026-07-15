// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/TestActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InteractableComponent.h"

// Sets default values
ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteract.AddDynamic(this, &ATestActor::OnHandledInteract);

		InteractableComponent->PromptText = TEXT("보물상자 열기 [F]");
	}
}

void ATestActor::OnHandledInteract(AActor* Interactor)
{
	if (Interactor)
	{
		UE_LOG(LogTemp, Warning, TEXT("상호작용"));
	}
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

