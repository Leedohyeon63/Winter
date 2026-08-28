#include "Actor/ItemStorageActor.h"

#include "Blueprint/UserWidget.h"
#include "Components/InteractableComponent.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/ItemStorageStateSubsystem.h"
#include "UI/ItemStorageWidget.h"

AItemStorageActor::AItemStorageActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(SceneRoot);
	InteractionCollision->InitSphereRadius(120.0f);
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
	StorageDisplayName = FText::FromString(TEXT("저장소"));
}

void AItemStorageActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteract.AddUniqueDynamic(this, &AItemStorageActor::HandleInteract);
	}

	LoadOrInitializeContents();
	UpdateInteractionState();
}

void AItemStorageActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseStorageUI();
	Super::EndPlay(EndPlayReason);
}

int32 AItemStorageActor::TransferStackToInventory(
	const int32 StackIndex,
	UPlayerInventoryComponent* TargetInventory)
{
	if (!TargetInventory || !Contents.IsValidIndex(StackIndex))
	{
		return 0;
	}

	const TObjectPtr<UItemDefinitionDataAsset> ItemDefinition = Contents[StackIndex].Item;
	const int32 RequestedQuantity = Contents[StackIndex].Quantity;
	if (!ItemDefinition || RequestedQuantity <= 0)
	{
		return 0;
	}

	const int32 AddedQuantity = TargetInventory->AddItem(ItemDefinition.Get(), RequestedQuantity);
	if (AddedQuantity <= 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[ItemStorage] Transfer blocked by player capacity. Storage=%s Item=%s Requested=%d"),
			*GetName(),
			*GetNameSafe(ItemDefinition.Get()),
			RequestedQuantity);
		return 0;
	}

	Contents[StackIndex].Quantity -= AddedQuantity;
	if (Contents[StackIndex].Quantity <= 0)
	{
		Contents.RemoveAt(StackIndex);
	}

	SaveContentsState();
	UpdateInteractionState();
	OnContentsChanged.Broadcast();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemStorage] Transferred. Storage=%s Item=%s Added=%d RemainingStacks=%d"),
		*GetName(),
		*GetNameSafe(ItemDefinition.Get()),
		AddedQuantity,
		Contents.Num());

	return AddedQuantity;
}

void AItemStorageActor::HandleInteract(AActor* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	UPlayerInventoryComponent* PlayerInventory =
		Interactor->FindComponentByClass<UPlayerInventoryComponent>();
	const APawn* InteractingPawn = Cast<APawn>(Interactor);
	APlayerController* PlayerController = InteractingPawn
		? Cast<APlayerController>(InteractingPawn->GetController())
		: nullptr;

	if (!PlayerInventory || !PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemStorage] Player inventory or controller was not found."));
		return;
	}

	OpenStorageUI(PlayerController, PlayerInventory);
}

void AItemStorageActor::HandleStorageWidgetCloseRequested()
{
	CloseStorageUI();
}

void AItemStorageActor::LoadOrInitializeContents()
{
	if (bPersistDuringSession)
	{
		UGameInstance* GameInstance = GetGameInstance();
		UItemStorageStateSubsystem* StateSubsystem = GameInstance
			? GameInstance->GetSubsystem<UItemStorageStateSubsystem>()
			: nullptr;

		if (StateSubsystem)
		{
			const FName StateKey = BuildStorageStateKey();
			if (const FItemStorageRuntimeState* ExistingState = StateSubsystem->FindStorageState(StateKey))
			{
				Contents = ExistingState->Contents;
				NormalizeContents();
				return;
			}
		}
	}

	Contents = InitialContents;
	NormalizeContents();
	SaveContentsState();
}

void AItemStorageActor::NormalizeContents()
{
	Contents.RemoveAll([](const FInventoryStack& Stack)
	{
		return !Stack.IsValid();
	});
}

void AItemStorageActor::SaveContentsState()
{
	if (!bPersistDuringSession)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (UItemStorageStateSubsystem* StateSubsystem = GameInstance
		? GameInstance->GetSubsystem<UItemStorageStateSubsystem>()
		: nullptr)
	{
		StateSubsystem->SaveStorageState(BuildStorageStateKey(), Contents);
	}
}

void AItemStorageActor::UpdateInteractionState()
{
	if (!InteractableComponent)
	{
		return;
	}

	const bool bIsEmpty = Contents.IsEmpty();
	InteractableComponent->SetInteractionEnabled(!bIsEmpty || !bDisableInteractionWhenEmpty);
	InteractableComponent->PromptText = bIsEmpty
		? FString::Printf(TEXT("%s - 비어 있음"), *StorageDisplayName.ToString())
		: FString::Printf(TEXT("%s 열기 [F]"), *StorageDisplayName.ToString());
}

void AItemStorageActor::OpenStorageUI(
	APlayerController* PlayerController,
	UPlayerInventoryComponent* PlayerInventory)
{
	if (!PlayerController || !PlayerInventory)
	{
		return;
	}

	if (!StorageWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemStorage] StorageWidgetClass is not set. Storage=%s"), *GetName());
		return;
	}

	if (IsValid(ActiveStorageWidget))
	{
		return;
	}

	ActiveStorageWidget = CreateWidget<UItemStorageWidget>(PlayerController, StorageWidgetClass);
	if (!ActiveStorageWidget)
	{
		return;
	}

	ActivePlayerController = PlayerController;
	ActiveStorageWidget->SetStorageContext(this, PlayerInventory);
	ActiveStorageWidget->OnCloseRequested.AddUniqueDynamic(
		this,
		&AItemStorageActor::HandleStorageWidgetCloseRequested);
	ActiveStorageWidget->AddToViewport(100);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ActiveStorageWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
}

void AItemStorageActor::CloseStorageUI()
{
	if (ActiveStorageWidget)
	{
		ActiveStorageWidget->OnCloseRequested.RemoveDynamic(
			this,
			&AItemStorageActor::HandleStorageWidgetCloseRequested);
		ActiveStorageWidget->RemoveFromParent();
		ActiveStorageWidget = nullptr;
	}

	if (ActivePlayerController)
	{
		FInputModeGameOnly InputMode;
		ActivePlayerController->SetInputMode(InputMode);
		ActivePlayerController->SetShowMouseCursor(false);
		ActivePlayerController->ResetIgnoreMoveInput();
		ActivePlayerController->ResetIgnoreLookInput();
		ActivePlayerController = nullptr;
	}
}

FName AItemStorageActor::BuildStorageStateKey() const
{
	FString MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("UnknownMap");
	if (GetWorld())
	{
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	}

	const FName EffectiveStorageId = StorageId.IsNone() ? GetFName() : StorageId;
	return FName(*FString::Printf(TEXT("%s::%s"), *MapName, *EffectiveStorageId.ToString()));
}
