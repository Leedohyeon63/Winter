#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "EquipmentSlotWidget.generated.h"

class UButton;
class UImage;
class UPlayerInventoryComponent;
class UTextBlock;

/** 장비 슬롯 하나를 표시하고 클릭 시 장비 해제를 요청한다. */
UCLASS()
class WINTER_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// [인벤토리 UI 추가] 장비 슬롯 종류와 현재 장착 아이템을 위젯에 연결한다.
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ConfigureEquipmentSlot(
		UPlayerInventoryComponent* InInventory,
		EEquipmentSlot InEquipmentSlot,
		UItemDefinitionDataAsset* InItem);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

private:
	UFUNCTION()
	void HandleSlotClicked();

	void RefreshVisuals();

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UItemDefinitionDataAsset> EquippedItem;

	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|UI", meta = (ClampMin = "0.1"))
	float DoubleClickThreshold = 0.3f;

	double LastClickTime = -1.0;
};
