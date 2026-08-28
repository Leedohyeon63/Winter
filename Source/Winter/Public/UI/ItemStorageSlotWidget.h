#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Struct/InventoryTypeStruct.h"
#include "ItemStorageSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnStorageTransferRequestedSignature,
	int32,
	StackIndex);

UCLASS()
class WINTER_API UItemStorageSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item Storage|UI")
	void ConfigureSlot(int32 InStackIndex, const FInventoryStack& InStack);

	UPROPERTY(BlueprintAssignable, Category = "Item Storage|UI")
	FOnStorageTransferRequestedSignature OnTransferRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Storage|UI", meta = (ClampMin = "0.1"))
	float DoubleClickThreshold = 0.3f;

private:
	UFUNCTION()
	void HandleSlotClicked();

	void RefreshVisuals();

	int32 StackIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FInventoryStack Stack;

	double LastClickTime = -1.0;
};
