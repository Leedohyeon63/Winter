#include "UI/ItemStorageWidget.h"

#include "Actor/ItemStorageActor.h"
#include "Components/Button.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Struct/InventoryTypeStruct.h"
#include "UI/ItemStorageSlotWidget.h"

void UItemStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UItemStorageWidget::HandleCloseClicked);
	}

	RefreshStorage();
}

void UItemStorageWidget::NativeDestruct()
{
	if (StorageActor)
	{
		StorageActor->OnContentsChanged.RemoveDynamic(this, &UItemStorageWidget::HandleStorageChanged);
	}

	Super::NativeDestruct();
}

void UItemStorageWidget::SetStorageContext(
	AItemStorageActor* InStorageActor,
	UPlayerInventoryComponent* InPlayerInventory)
{
	if (StorageActor)
	{
		StorageActor->OnContentsChanged.RemoveDynamic(this, &UItemStorageWidget::HandleStorageChanged);
	}

	StorageActor = InStorageActor;
	PlayerInventory = InPlayerInventory;

	if (StorageActor)
	{
		StorageActor->OnContentsChanged.AddUniqueDynamic(this, &UItemStorageWidget::HandleStorageChanged);
	}

	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshStorage();
}

void UItemStorageWidget::RefreshStorage()
{
	if (!StorageGrid)
	{
		return;
	}

	StorageGrid->ClearChildren();

	if (StorageNameText)
	{
		StorageNameText->SetText(StorageActor
			? StorageActor->GetStorageDisplayName()
			: FText::GetEmpty());
	}

	const TArray<FInventoryStack> StorageContents = StorageActor
		? StorageActor->GetContents()
		: TArray<FInventoryStack>();
	const bool bIsEmpty = StorageContents.IsEmpty();

	if (EmptyText)
	{
		EmptyText->SetText(FText::FromString(TEXT("비어 있음")));
		EmptyText->SetVisibility(bIsEmpty
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!StorageActor || !StorageSlotWidgetClass)
	{
		if (StorageActor && !StorageSlotWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemStorageUI] StorageSlotWidgetClass is not set."));
		}
		return;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	const int32 SafeColumns = FMath::Max(1, StorageColumns);

	for (int32 StackIndex = 0; StackIndex < StorageContents.Num(); ++StackIndex)
	{
		UItemStorageSlotWidget* SlotWidget =
			CreateWidget<UItemStorageSlotWidget>(PlayerController, StorageSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->ConfigureSlot(StackIndex, StorageContents[StackIndex]);
		SlotWidget->OnTransferRequested.AddUniqueDynamic(
			this,
			&UItemStorageWidget::HandleTransferRequested);
		StorageGrid->AddChildToUniformGrid(
			SlotWidget,
			StackIndex / SafeColumns,
			StackIndex % SafeColumns);
	}
}

void UItemStorageWidget::HandleStorageChanged()
{
	RefreshStorage();
}

void UItemStorageWidget::HandleTransferRequested(const int32 StackIndex)
{
	if (!StorageActor || !PlayerInventory)
	{
		return;
	}

	const TArray<FInventoryStack> BeforeContents = StorageActor->GetContents();
	if (!BeforeContents.IsValidIndex(StackIndex))
	{
		return;
	}

	const FText ItemName = BeforeContents[StackIndex].Item
		? BeforeContents[StackIndex].Item->DisplayName
		: FText::GetEmpty();
	const int32 AddedQuantity = StorageActor->TransferStackToInventory(StackIndex, PlayerInventory);

	if (!StatusText)
	{
		return;
	}

	if (AddedQuantity > 0)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("%s x%d 획득"),
			*ItemName.ToString(),
			AddedQuantity)));
	}
	else
	{
		StatusText->SetText(FText::FromString(TEXT("인벤토리 공간 또는 무게가 부족합니다.")));
	}
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UItemStorageWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

APlayerController* UItemStorageWidget::ResolvePlayerController() const
{
	return GetOwningPlayer()
		? GetOwningPlayer()
		: UGameplayStatics::GetPlayerController(this, 0);
}
