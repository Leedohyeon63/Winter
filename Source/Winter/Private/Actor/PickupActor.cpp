#include "Actor/PickupActor.h"
#include "Components/InteractableComponent.h"
#include "Components/SphereComponent.h" 
#include "Components/StaticMeshComponent.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/SceneComponent.h"


APickupActor::APickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// [픽업 컴포넌트 구조 수정] 메시를 Root로 사용하지 않아 메시 스케일이 콜리전에 전달되지 않게 한다.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));

	// [픽업 컴포넌트 구조 수정] 콜리전을 메시가 아닌 공통 SceneRoot에 부착한다.
	InteractionCollision->SetupAttachment(SceneRoot);
	InteractionCollision->InitSphereRadius(100.0f);

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);

	InteractableComponent =
		CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
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