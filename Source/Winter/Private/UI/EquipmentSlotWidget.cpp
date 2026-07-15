#include "UI/EquipmentSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h" // [인벤토리 UI 추가] 장착 아이템의 Soft Texture 아이콘을 실제 브러시로 설정한다.
#include "HAL/PlatformTime.h"

void UEquipmentSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// [인벤토리 UI 추가] 장착 아이템이 있는 장비 슬롯을 클릭하면 해제하도록 연결한다.
	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleSlotClicked);
	}
}

void UEquipmentSlotWidget::ConfigureEquipmentSlot(
	UPlayerInventoryComponent* InInventory,
	EEquipmentSlot InEquipmentSlot,
	UItemDefinitionDataAsset* InItem)
{
	InventoryComponent = InInventory;
	EquipmentSlot = InEquipmentSlot;
	EquippedItem = InItem;
	LastClickTime = -1.0; // [인벤토리 UI 추가] 장착 정보가 바뀌면 이전 아이템의 클릭 상태를 폐기한다.
	RefreshVisuals();
}

void UEquipmentSlotWidget::HandleSlotClicked()
{
	const double CurrentClickTime = FPlatformTime::Seconds();
	const bool bIsDoubleClick = LastClickTime >= 0.0
		&& CurrentClickTime - LastClickTime <= DoubleClickThreshold;

	// [인벤토리 UI 추가] 장비 슬롯은 두 번 클릭했을 때만 해제하며, 용량 부족 시 컴포넌트가 거부한다.
	if (bIsDoubleClick && InventoryComponent && EquippedItem)
	{
		InventoryComponent->UnequipItem(EquipmentSlot);
		LastClickTime = -1.0;
		return;
	}

	LastClickTime = CurrentClickTime;
}

void UEquipmentSlotWidget::RefreshVisuals()
{
	const bool bHasItem = EquippedItem != nullptr;

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(bHasItem);
	}

	if (SlotNameText)
	{
		const UEnum* EquipmentEnum = StaticEnum<EEquipmentSlot>();
		SlotNameText->SetText(EquipmentEnum
			? EquipmentEnum->GetDisplayNameTextByValue(static_cast<int64>(EquipmentSlot))
			: FText::GetEmpty());
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(bHasItem ? EquippedItem->DisplayName : FText::FromString(TEXT("비어 있음")));
	}

	if (ItemIcon)
	{
		if (bHasItem && !EquippedItem->Icon.IsNull())
		{
			ItemIcon->SetBrushFromTexture(EquippedItem->Icon.LoadSynchronous());
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetBrushFromTexture(nullptr);
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
