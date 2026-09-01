#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Struct/InventoryTypeStruct.h"
#include "Struct/PlayerTravelState.h"
#include "PlayerInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCarryWeightChangedSignature, float, CurrentWeight, float, MaxWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryCapacityChangedSignature, int32, UsedSlots, int32, MaxSlots);

/**
 * 싱글플레이용 플레이어 인벤토리.
 * 배열의 한 원소가 UI의 한 슬롯이며 같은 아이템은 MaxStackSize만큼 쌓인다.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class WINTER_API UPlayerInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChangedSignature OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnCarryWeightChangedSignature OnCarryWeightChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryCapacityChangedSignature OnCapacityChanged;

	/** 가능한 수량만 추가하고 실제 추가된 개수를 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDefinitionDataAsset* Item, int32 Quantity = 1);

	/** 요청 수량을 전부 제거할 수 있을 때만 성공한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemDefinitionDataAsset* Item, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(UItemDefinitionDataAsset* Item, int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAddableQuantity(UItemDefinitionDataAsset* Item, int32 RequestedQuantity) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemQuantity(UItemDefinitionDataAsset* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Capacity")
	int32 GetUsedSlotCount() const { return Items.Num(); }

	UFUNCTION(BlueprintPure, Category = "Inventory|Capacity")
	int32 GetSlotCapacity() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetInventoryWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetEquipmentWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetTotalCarriedWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetMaxCarryWeight() const;

	/** 인벤토리에 있는 아이템 한 개를 지정된 장비 슬롯에 장착한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool EquipItem(UItemDefinitionDataAsset* Item);

	/** 장비를 인벤토리로 되돌린 뒤에도 공간/무게 제한을 만족할 때만 해제한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool UnequipItem(EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	UItemDefinitionDataAsset* GetEquippedItem(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	TMap<EEquipmentSlot, UItemDefinitionDataAsset*> GetEquippedItems() const;

	// [레벨 이동 추가] GameInstance 서브시스템이 레벨 이동 전후에 같은 인벤토리를 복원할 때 사용한다.
	FInventoryTravelState CaptureTravelState() const;
	void RestoreTravelState(const FInventoryTravelState& InState);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Capacity", meta = (ClampMin = "0"))
	int32 BaseSlotCapacity = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Capacity", meta = (ClampMin = "0.0"))
	float BaseMaxCarryWeight = 10.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryStack> Items;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory|Equipment")
	TMap<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>> EquippedItems;

private:
	void AddItemUnchecked(UItemDefinitionDataAsset* Item, int32 Quantity);
	bool RemoveItemInternal(UItemDefinitionDataAsset* Item, int32 Quantity);
	bool IsCurrentStateWithinCapacity() const;
	void BroadcastStateChanged();
};
