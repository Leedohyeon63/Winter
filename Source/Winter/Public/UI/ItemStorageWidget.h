#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemStorageWidget.generated.h"

class AItemStorageActor;
class APlayerController;
class UButton;
class UItemStorageSlotWidget;
class UPlayerInventoryComponent;
class UTextBlock;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemStorageCloseRequestedSignature);

UCLASS()
class WINTER_API UItemStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item Storage|UI")
	void SetStorageContext(
		AItemStorageActor* InStorageActor,
		UPlayerInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "Item Storage|UI")
	void RefreshStorage();

	UPROPERTY(BlueprintAssignable, Category = "Item Storage|UI")
	FOnItemStorageCloseRequestedSignature OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> StorageGrid;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StorageNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Storage|UI")
	TSubclassOf<UItemStorageSlotWidget> StorageSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Storage|UI", meta = (ClampMin = "1"))
	int32 StorageColumns = 5;

private:
	UFUNCTION()
	void HandleStorageChanged();

	UFUNCTION()
	void HandleTransferRequested(int32 StackIndex);

	UFUNCTION()
	void HandleCloseClicked();

	APlayerController* ResolvePlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<AItemStorageActor> StorageActor;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInventoryComponent> PlayerInventory;
};
