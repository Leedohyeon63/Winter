// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerWidget.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "UI/PlayerInventoryWidget.h"

void UPlayerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BoundPlayerCharacter = ResolvePlayerCharacter();
	if (BoundPlayerCharacter)
	{
		BoundPlayerCharacter->OnInventoryToggleRequested.AddUniqueDynamic(
			this,
			&UPlayerWidget::HandleInventoryToggleRequested);

		if (InventoryPanel)
		{
			InventoryPanel->SetInventoryComponent(BoundPlayerCharacter->GetInventoryComponent());
		}
	}

	if (InventoryPanel)
	{
		InventoryPanel->OnCloseRequested.AddUniqueDynamic(this, &UPlayerWidget::HandleInventoryCloseRequested);
	}

	SetInventoryOpen(false);
}

void UPlayerWidget::NativeDestruct()
{
	if (BoundPlayerCharacter)
	{
		BoundPlayerCharacter->OnInventoryToggleRequested.RemoveDynamic(
			this,
			&UPlayerWidget::HandleInventoryToggleRequested);
	}

	if (InventoryPanel)
	{
		InventoryPanel->OnCloseRequested.RemoveDynamic(this, &UPlayerWidget::HandleInventoryCloseRequested);
	}

	SetInventoryOpen(false);
	Super::NativeDestruct();
}

void UPlayerWidget::ToggleInventory()
{
	SetInventoryOpen(!bInventoryOpen);
}

void UPlayerWidget::SetInventoryOpen(bool bOpen)
{
	if (!InventoryPanel)
	{
		bInventoryOpen = false;
		return;
	}

	bInventoryOpen = bOpen;
	InventoryPanel->SetVisibility(bInventoryOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	APlayerController* PlayerController = GetOwningPlayer()
		? GetOwningPlayer()
		: UGameplayStatics::GetPlayerController(this, 0);

	if (!PlayerController)
	{
		return;
	}

	if (bInventoryOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		InventoryPanel->RefreshInventory();
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(false);
		PlayerController->ResetIgnoreMoveInput();
		PlayerController->ResetIgnoreLookInput();
	}
}

void UPlayerWidget::HandleInventoryToggleRequested()
{
	ToggleInventory();
}

void UPlayerWidget::HandleInventoryCloseRequested()
{
	SetInventoryOpen(false);
}

APlayerCharacter* UPlayerWidget::ResolvePlayerCharacter() const
{
	if (APlayerCharacter* OwningCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		return OwningCharacter;
	}

	return Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}
