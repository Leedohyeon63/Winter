// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerWidget.generated.h"

class APlayerCharacter;
class UPlayerInventoryWidget;

/**
 * 
 */
UCLASS()
class WINTER_API UPlayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetInventoryOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	bool IsInventoryOpen() const { return bInventoryOpen; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPlayerInventoryWidget> InventoryPanel;

private:
	UFUNCTION()
	void HandleInventoryToggleRequested();

	UFUNCTION()
	void HandleInventoryCloseRequested();

	APlayerCharacter* ResolvePlayerCharacter() const;

	UPROPERTY(Transient)
	TObjectPtr<APlayerCharacter> BoundPlayerCharacter;

	bool bInventoryOpen = false;
};
