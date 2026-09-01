#include "Components/PlayerInventoryComponent.h"

UPlayerInventoryComponent::UPlayerInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UPlayerInventoryComponent::AddItem(UItemDefinitionDataAsset* Item, int32 Quantity)
{
	const int32 QuantityToAdd = GetAddableQuantity(Item, Quantity);
	if (QuantityToAdd <= 0)
	{
		return 0;
	}

	AddItemUnchecked(Item, QuantityToAdd);
	BroadcastStateChanged();
	return QuantityToAdd;
}

bool UPlayerInventoryComponent::RemoveItem(UItemDefinitionDataAsset* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0 || GetItemQuantity(Item) < Quantity)
	{
		return false;
	}

	if (!RemoveItemInternal(Item, Quantity))
	{
		return false;
	}

	BroadcastStateChanged();
	return true;
}

bool UPlayerInventoryComponent::CanAddItem(UItemDefinitionDataAsset* Item, int32 Quantity) const
{
	return Quantity > 0 && GetAddableQuantity(Item, Quantity) == Quantity;
}

int32 UPlayerInventoryComponent::GetAddableQuantity(UItemDefinitionDataAsset* Item, int32 RequestedQuantity) const
{
	if (!Item || RequestedQuantity <= 0)
	{
		return 0;
	}

	const int32 MaxStackSize = FMath::Max(1, Item->MaxStackSize);
	int32 SlotSpace = 0;

	for (const FInventoryStack& Stack : Items)
	{
		if (Stack.Item == Item)
		{
			SlotSpace += FMath::Max(0, MaxStackSize - Stack.Quantity);
		}
	}

	const int32 FreeSlots = FMath::Max(0, GetSlotCapacity() - Items.Num());
	SlotSpace += FreeSlots * MaxStackSize;

	int32 WeightSpace = RequestedQuantity;
	if (Item->UnitWeight > KINDA_SMALL_NUMBER)
	{
		const float RemainingWeight = FMath::Max(0.0f, GetMaxCarryWeight() - GetTotalCarriedWeight());
		WeightSpace = FMath::FloorToInt((RemainingWeight + KINDA_SMALL_NUMBER) / Item->UnitWeight);
	}

	return FMath::Clamp(FMath::Min3(RequestedQuantity, SlotSpace, WeightSpace), 0, RequestedQuantity);
}

int32 UPlayerInventoryComponent::GetItemQuantity(UItemDefinitionDataAsset* Item) const
{
	if (!Item)
	{
		return 0;
	}

	int32 TotalQuantity = 0;
	for (const FInventoryStack& Stack : Items)
	{
		if (Stack.Item == Item)
		{
			TotalQuantity += Stack.Quantity;
		}
	}
	return TotalQuantity;
}

int32 UPlayerInventoryComponent::GetSlotCapacity() const
{
	int32 Capacity = BaseSlotCapacity;
	for (const TPair<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>>& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			Capacity += Pair.Value->GrantedSlots;
		}
	}
	return FMath::Max(0, Capacity);
}

float UPlayerInventoryComponent::GetInventoryWeight() const
{
	float Weight = 0.0f;
	for (const FInventoryStack& Stack : Items)
	{
		Weight += Stack.GetWeight();
	}
	return FMath::Max(0.0f, Weight);
}

float UPlayerInventoryComponent::GetEquipmentWeight() const
{
	float Weight = 0.0f;
	for (const TPair<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>>& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			Weight += Pair.Value->UnitWeight;
		}
	}
	return FMath::Max(0.0f, Weight);
}

float UPlayerInventoryComponent::GetTotalCarriedWeight() const
{
	return GetInventoryWeight() + GetEquipmentWeight();
}

float UPlayerInventoryComponent::GetMaxCarryWeight() const
{
	float MaxWeight = BaseMaxCarryWeight;
	for (const TPair<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>>& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			MaxWeight += Pair.Value->GrantedMaxWeight;
		}
	}
	return FMath::Max(0.0f, MaxWeight);
}

bool UPlayerInventoryComponent::EquipItem(UItemDefinitionDataAsset* Item)
{
	if (!Item || !Item->bEquippable || Item->EquipmentSlot == EEquipmentSlot::None || GetItemQuantity(Item) <= 0)
	{
		return false;
	}

	const TArray<FInventoryStack> OriginalItems = Items;
	const TMap<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>> OriginalEquipment = EquippedItems;

	RemoveItemInternal(Item, 1);

	if (UItemDefinitionDataAsset* PreviouslyEquipped = GetEquippedItem(Item->EquipmentSlot))
	{
		AddItemUnchecked(PreviouslyEquipped, 1);
	}

	EquippedItems.Add(Item->EquipmentSlot, Item);

	if (!IsCurrentStateWithinCapacity())
	{
		Items = OriginalItems;
		EquippedItems = OriginalEquipment;
		return false;
	}

	BroadcastStateChanged();
	return true;
}

bool UPlayerInventoryComponent::UnequipItem(EEquipmentSlot Slot)
{
	UItemDefinitionDataAsset* Item = GetEquippedItem(Slot);
	if (!Item)
	{
		return false;
	}

	const TArray<FInventoryStack> OriginalItems = Items;
	const TMap<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>> OriginalEquipment = EquippedItems;

	EquippedItems.Remove(Slot);
	AddItemUnchecked(Item, 1);

	if (!IsCurrentStateWithinCapacity())
	{
		Items = OriginalItems;
		EquippedItems = OriginalEquipment;
		return false;
	}

	BroadcastStateChanged();
	return true;
}

UItemDefinitionDataAsset* UPlayerInventoryComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	if (const TObjectPtr<UItemDefinitionDataAsset>* FoundItem = EquippedItems.Find(Slot))
	{
		return FoundItem->Get();
	}
	return nullptr;
}

TMap<EEquipmentSlot, UItemDefinitionDataAsset*> UPlayerInventoryComponent::GetEquippedItems() const
{
	TMap<EEquipmentSlot, UItemDefinitionDataAsset*> Result;
	for (const TPair<EEquipmentSlot, TObjectPtr<UItemDefinitionDataAsset>>& Pair : EquippedItems)
	{
		Result.Add(Pair.Key, Pair.Value.Get());
	}
	return Result;
}

FInventoryTravelState UPlayerInventoryComponent::CaptureTravelState() const
{
	// [레벨 이동 추가] 아이템 스택과 장착 상태를 한 시점의 값으로 복사한다.
	FInventoryTravelState State;
	State.Items = Items;
	State.EquippedItems = EquippedItems;
	return State;
}

void UPlayerInventoryComponent::RestoreTravelState(const FInventoryTravelState& InState)
{
	// [레벨 이동 추가] 같은 플레이 세션에서 캡처한 유효한 상태를 그대로 복원한다.
	Items = InState.Items;
	EquippedItems = InState.EquippedItems;
	BroadcastStateChanged();
}

void UPlayerInventoryComponent::AddItemUnchecked(UItemDefinitionDataAsset* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0)
	{
		return;
	}

	const int32 MaxStackSize = FMath::Max(1, Item->MaxStackSize);
	int32 Remaining = Quantity;

	for (FInventoryStack& Stack : Items)
	{
		if (Stack.Item != Item || Stack.Quantity >= MaxStackSize)
		{
			continue;
		}

		const int32 AddedToStack = FMath::Min(Remaining, MaxStackSize - Stack.Quantity);
		Stack.Quantity += AddedToStack;
		Remaining -= AddedToStack;

		if (Remaining <= 0)
		{
			return;
		}
	}

	while (Remaining > 0)
	{
		FInventoryStack& NewStack = Items.AddDefaulted_GetRef();
		NewStack.Item = Item;
		NewStack.Quantity = FMath::Min(Remaining, MaxStackSize);
		Remaining -= NewStack.Quantity;
	}
}

bool UPlayerInventoryComponent::RemoveItemInternal(UItemDefinitionDataAsset* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0 || GetItemQuantity(Item) < Quantity)
	{
		return false;
	}

	int32 Remaining = Quantity;
	for (int32 Index = Items.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FInventoryStack& Stack = Items[Index];
		if (Stack.Item != Item)
		{
			continue;
		}

		const int32 RemovedFromStack = FMath::Min(Remaining, Stack.Quantity);
		Stack.Quantity -= RemovedFromStack;
		Remaining -= RemovedFromStack;

		if (Stack.Quantity <= 0)
		{
			Items.RemoveAt(Index);
		}
	}

	return Remaining == 0;
}

bool UPlayerInventoryComponent::IsCurrentStateWithinCapacity() const
{
	return Items.Num() <= GetSlotCapacity()
		&& GetTotalCarriedWeight() <= GetMaxCarryWeight() + KINDA_SMALL_NUMBER;
}

void UPlayerInventoryComponent::BroadcastStateChanged()
{
	OnInventoryChanged.Broadcast();
	OnCarryWeightChanged.Broadcast(GetTotalCarriedWeight(), GetMaxCarryWeight());
	OnCapacityChanged.Broadcast(GetUsedSlotCount(), GetSlotCapacity());
}
