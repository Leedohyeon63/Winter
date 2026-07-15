#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Struct/InventoryTypeStruct.h"
#include "InventorySlotWidget.generated.h"

class UButton;
class UImage;
class UPlayerInventoryComponent;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemClickedSignature, UItemDefinitionDataAsset*, Item);

/** 인벤토리의 한 스택 또는 빈 슬롯을 표시한다. */
UCLASS()
class WINTER_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ConfigureSlot(UPlayerInventoryComponent* InInventory, const FInventoryStack& InStack);

	UPROPERTY(BlueprintAssignable, Category = "Inventory|UI")
	FOnInventoryItemClickedSignature OnItemClicked;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	bool bEquipOnDoubleClick = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI", meta = (ClampMin = "0.1"))
	float DoubleClickThreshold = 0.3f;

private:
	UFUNCTION()
	void HandleSlotClicked();

	void RefreshVisuals();

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	FInventoryStack Stack;

	double LastClickTime = -1.0;
};
