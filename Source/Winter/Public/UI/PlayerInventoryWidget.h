#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerInventoryWidget.generated.h"

class UButton;
class UEquipmentSlotWidget;
class UInventorySlotWidget;
class UPlayerInventoryComponent;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryCloseRequestedSignature);

/** 플레이어 인벤토리 전체 패널. 슬롯과 장비 위젯을 현재 용량에 맞춰 동적으로 만든다. */
UCLASS()
class WINTER_API UPlayerInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// [인벤토리 UI 추가] Main HUD가 플레이어의 실제 인벤토리 컴포넌트를 전달한다.
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetInventoryComponent(UPlayerInventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void RefreshInventory();

	UPROPERTY(BlueprintAssignable, Category = "Inventory|UI")
	FOnInventoryCloseRequestedSignature OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// [인벤토리 UI 추가] WBP_PlayerInventory 디자이너에서 정확히 같은 이름을 사용한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> EquipmentList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CapacityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UEquipmentSlotWidget> EquipmentSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI", meta = (ClampMin = "1"))
	int32 InventoryColumns = 5;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleCloseClicked();

	void BuildInventorySlots();
	void BuildEquipmentSlots();
	void RefreshSummary();
	APlayerController* ResolvePlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInventoryComponent> InventoryComponent;
};
