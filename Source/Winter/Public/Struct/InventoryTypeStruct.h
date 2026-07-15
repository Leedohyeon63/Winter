// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "InventoryTypeStruct.generated.h"

USTRUCT(BlueprintType)
struct WINTER_API FInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemDefinitionDataAsset> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	bool IsValid() const
	{
		return Item != nullptr && Quantity > 0;
	}

	float GetWeight() const
	{
		return Item ? Item->UnitWeight * Quantity : 0.0f;
	}
};