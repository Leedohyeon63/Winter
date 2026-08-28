#include "UI/ItemStorageSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "HAL/PlatformTime.h"

void UItemStorageSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UItemStorageSlotWidget::HandleSlotClicked);
	}
}

void UItemStorageSlotWidget::ConfigureSlot(
	const int32 InStackIndex,
	const FInventoryStack& InStack)
{
	StackIndex = InStackIndex;
	Stack = InStack;
	LastClickTime = -1.0;
	RefreshVisuals();
}

void UItemStorageSlotWidget::HandleSlotClicked()
{
	if (!Stack.IsValid() || StackIndex == INDEX_NONE)
	{
		return;
	}

	const double CurrentClickTime = FPlatformTime::Seconds();
	const bool bIsDoubleClick = LastClickTime >= 0.0
		&& CurrentClickTime - LastClickTime <= DoubleClickThreshold;

	if (bIsDoubleClick)
	{
		OnTransferRequested.Broadcast(StackIndex);
		LastClickTime = -1.0;
		return;
	}

	LastClickTime = CurrentClickTime;
}

void UItemStorageSlotWidget::RefreshVisuals()
{
	const bool bHasItem = Stack.IsValid();

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(bHasItem);
	}

	if (ItemIcon)
	{
		if (bHasItem && !Stack.Item->Icon.IsNull())
		{
			ItemIcon->SetBrushFromTexture(Stack.Item->Icon.LoadSynchronous());
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetBrushFromTexture(nullptr);
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (QuantityText)
	{
		QuantityText->SetText(bHasItem ? FText::AsNumber(Stack.Quantity) : FText::GetEmpty());
		QuantityText->SetVisibility(bHasItem && Stack.Quantity > 1
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(bHasItem ? Stack.Item->DisplayName : FText::GetEmpty());
		ItemNameText->SetVisibility(bHasItem
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (WeightText)
	{
		const float StackWeight = bHasItem ? Stack.GetWeight() : 0.0f;
		WeightText->SetText(bHasItem
			? FText::FromString(FString::Printf(TEXT("%.1f kg"), StackWeight))
			: FText::GetEmpty());
		WeightText->SetVisibility(bHasItem
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
