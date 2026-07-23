#include "Actor/LevelPortal.h"
#include "Components/ArrowComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DataAsset/PortalDefinitionDataAsset.h"
#include "Engine/GameInstance.h"
#include "PlayerCharacter.h"
#include "Subsystem/LevelTravelSubsystem.h"

ALevelPortal::ALevelPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(MeshComponent);
	InteractionCollision->InitSphereRadius(150.0f);
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));

	ArrivalPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(MeshComponent);
	ArrivalPoint->SetRelativeLocation(FVector(200.0f, 0.0f, 0.0f));
}

void ALevelPortal::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteract.AddDynamic(this, &ALevelPortal::HandleInteract);
	}

	RefreshInteractionState();
}

FName ALevelPortal::GetPortalId() const
{
	// [포탈 데이터 에셋 변경] 도착 포탈 검색도 배치 액터가 아닌 정의 에셋의 ID를 기준으로 한다.
	return PortalDefinition ? PortalDefinition->PortalId : NAME_None;
}

FTransform ALevelPortal::GetArrivalTransform() const
{
	return ArrivalPoint ? ArrivalPoint->GetComponentTransform() : GetActorTransform();
}

void ALevelPortal::HandleInteract(AActor* Interactor)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(Interactor);
	if (!Player || !InteractableComponent || !PortalDefinition)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULevelTravelSubsystem* TravelSubsystem = GameInstance
		? GameInstance->GetSubsystem<ULevelTravelSubsystem>()
		: nullptr;

	if (!TravelSubsystem)
	{
		return;
	}

	InteractableComponent->SetInteractionEnabled(false);
	if (!TravelSubsystem->TravelToLevel(
		Player,
		PortalDefinition->TargetLevel,
		PortalDefinition->TargetPortalId))
	{
		InteractableComponent->SetInteractionEnabled(true);
	}
}

void ALevelPortal::RefreshInteractionState()
{
	if (!InteractableComponent)
	{
		return;
	}

	InteractableComponent->PromptText = PortalDefinition ? PortalDefinition->InteractionPrompt : TEXT("포탈 설정");
	InteractableComponent->SetInteractionEnabled(PortalDefinition && !PortalDefinition->TargetLevel.IsNull());
}
