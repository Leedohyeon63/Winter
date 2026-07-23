#pragma once

#include "CoreMinimal.h"
#include "Struct/InventoryTypeStruct.h"
#include "PlayerTravelState.generated.h"

USTRUCT()
struct WINTER_API FInventoryTravelState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryStack> Items;

	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>> EquippedItems;
};

USTRUCT()
struct WINTER_API FPlayerTravelState
{
	GENERATED_BODY()

	UPROPERTY()
	FInventoryTravelState Inventory;

	UPROPERTY()
	float Health = 100.0f;

	UPROPERTY()
	float MaxHealth = 100.0f;

	UPROPERTY()
	float Stamina = 50.0f;

	UPROPERTY()
	float MaxStamina = 50.0f;

	UPROPERTY()
	float Mentality = 100.0f;

	UPROPERTY()
	float MaxMentality = 100.0f;

	UPROPERTY()
	bool bIsValid = false;
};
