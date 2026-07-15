#include "UI/PlayerInventoryWidget.h"

#include "Components/Button.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "UI/EquipmentSlotWidget.h"
#include "UI/InventorySlotWidget.h"

void UPlayerInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UPlayerInventoryWidget::HandleCloseClicked);
	}

	if (!InventoryComponent)
	{
		// [인벤토리 UI 추가] HUD가 Owning Player를 지정하지 않은 기존 Blueprint 흐름도 지원한다.
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			SetInventoryComponent(Player->GetInventoryComponent());
		}
	}

	RefreshInventory();
}

void UPlayerInventoryWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPlayerInventoryWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UPlayerInventoryWidget::SetInventoryComponent(UPlayerInventoryComponent* InInventoryComponent)
{
	if (InventoryComponent == InInventoryComponent)
	{
		RefreshInventory();
		return;
	}

	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPlayerInventoryWidget::HandleInventoryChanged);
	}

	InventoryComponent = InInventoryComponent;

	if (InventoryComponent)
	{
		// [인벤토리 UI 추가] 아이템 획득·장착·해제 시 전체 UI가 자동 갱신된다.
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPlayerInventoryWidget::HandleInventoryChanged);
	}

	RefreshInventory();
}

void UPlayerInventoryWidget::RefreshInventory()
{
	if (!InventoryComponent || !InventoryGrid || !EquipmentList)
	{
		return;
	}

	BuildInventorySlots();
	BuildEquipmentSlots();
	RefreshSummary();
}

void UPlayerInventoryWidget::HandleInventoryChanged()
{
	RefreshInventory();
}

void UPlayerInventoryWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void UPlayerInventoryWidget::BuildInventorySlots()
{
	InventoryGrid->ClearChildren();

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryUI] InventorySlotWidgetClass가 지정되지 않았습니다."));
		return;
	}

	const TArray<FInventoryStack> Items = InventoryComponent->GetItems();
	const int32 SlotCapacity = InventoryComponent->GetSlotCapacity();
	APlayerController* PlayerController = ResolvePlayerController();

	for (int32 Index = 0; Index < SlotCapacity; ++Index)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(PlayerController, InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		// [인벤토리 UI 추가] 실제 스택이 없는 위치도 빈 슬롯 위젯으로 생성해 배낭 전체 공간을 보여준다.
		const FInventoryStack Stack = Items.IsValidIndex(Index) ? Items[Index] : FInventoryStack();
		SlotWidget->ConfigureSlot(InventoryComponent, Stack);

		const int32 SafeColumns = FMath::Max(1, InventoryColumns);
		InventoryGrid->AddChildToUniformGrid(SlotWidget, Index / SafeColumns, Index % SafeColumns);
	}
}

void UPlayerInventoryWidget::BuildEquipmentSlots()
{
	EquipmentList->ClearChildren();

	if (!EquipmentSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryUI] EquipmentSlotWidgetClass가 지정되지 않았습니다."));
		return;
	}

	// [인벤토리 UI 추가] None을 제외한 고정 장비 슬롯을 항상 같은 순서로 표시한다.
	static const EEquipmentSlot Slots[] = {
		EEquipmentSlot::Head,
		EEquipmentSlot::Body,
		EEquipmentSlot::Hands,
		EEquipmentSlot::Legs,
		EEquipmentSlot::Feet,
		EEquipmentSlot::Backpack,
		EEquipmentSlot::PrimaryWeapon,
		EEquipmentSlot::SecondaryWeapon
	};

	APlayerController* PlayerController = ResolvePlayerController();
	for (const EEquipmentSlot EquipmentSlotType : Slots)
	{
		UEquipmentSlotWidget* SlotWidget = CreateWidget<UEquipmentSlotWidget>(PlayerController, EquipmentSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->ConfigureEquipmentSlot(InventoryComponent, EquipmentSlotType, InventoryComponent->GetEquippedItem(EquipmentSlotType));
		EquipmentList->AddChild(SlotWidget);
	}
}

void UPlayerInventoryWidget::RefreshSummary()
{
	if (WeightText)
	{
		WeightText->SetText(FText::FromString(FString::Printf(
			TEXT("무게 %.1f / %.1f kg"),
			InventoryComponent->GetTotalCarriedWeight(),
			InventoryComponent->GetMaxCarryWeight())));
	}

	if (CapacityText)
	{
		CapacityText->SetText(FText::FromString(FString::Printf(
			TEXT("공간 %d / %d"),
			InventoryComponent->GetUsedSlotCount(),
			InventoryComponent->GetSlotCapacity())));
	}
}

APlayerController* UPlayerInventoryWidget::ResolvePlayerController() const
{
	return GetOwningPlayer() ? GetOwningPlayer() : UGameplayStatics::GetPlayerController(this, 0);
}
