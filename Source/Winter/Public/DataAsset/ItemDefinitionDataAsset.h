// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinitionDataAsset.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None UMETA(DisplayName = "None"),
	Head UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body"),
	Hands UMETA(DisplayName = "Hands"),
	Legs UMETA(DisplayName = "Legs"),
	Feet UMETA(DisplayName = "Feet"),
	Backpack UMETA(DisplayName = "Backpack"),
	PrimaryWeapon UMETA(DisplayName = "Primary Weapon"),
	SecondaryWeapon UMETA(DisplayName = "Secondary Weapon")
};

/**
 * 
 */
UCLASS(BlueprintType)
class WINTER_API UItemDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float UnitWeight = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	bool bEquippable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "bEquippable"))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Storage", meta = (EditCondition = "bEquippable", ClampMin = "0"))
	int32 GrantedSlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Storage", meta = (EditCondition = "bEquippable", ClampMin = "0.0"))
	float GrantedMaxWeight = 0.0f;
};
