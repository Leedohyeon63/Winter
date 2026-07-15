#include "Actor/PickupActor.h"
#include "Components/InteractableComponent.h"
#include "Components/SphereComponent.h" 
#include "Components/StaticMeshComponent.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "Components/PlayerInventoryComponent.h"

APickupActor::APickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(MeshComponent);
	InteractionCollision->InitSphereRadius(100.0f);

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
}

void APickupActor::BeginPlay()
{
	Super::BeginPlay();

	InteractableComponent->OnInteract.AddDynamic(this, &APickupActor::HandleInteract);
	InteractableComponent->SetInteractionEnabled(ItemDefinition != nullptr && Quantity > 0);
	RefreshPrompt();
}

void APickupActor::HandleInteract(AActor* Interactor)
{
	if (!Interactor || !ItemDefinition || Quantity <= 0)
	{
		return;
	}

	UPlayerInventoryComponent* Inventory = Interactor->FindComponentByClass<UPlayerInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	const int32 AddedQuantity = Inventory->AddItem(ItemDefinition, Quantity);
	if (AddedQuantity <= 0)
	{
		return;
	}

	Quantity -= AddedQuantity;
	if (Quantity <= 0)
	{
		InteractableComponent->SetInteractionEnabled(false);
		Destroy();
		return;
	}

	RefreshPrompt();
}

void APickupActor::RefreshPrompt()
{
	if (!InteractableComponent || !ItemDefinition)
	{
		return;
	}

	InteractableComponent->PromptText = Quantity > 1
		? FString::Printf(TEXT("%s x%d 줍기 [F]"), *ItemDefinition->DisplayName.ToString(), Quantity)
		: FString::Printf(TEXT("%s 줍기 [F]"), *ItemDefinition->DisplayName.ToString());
}