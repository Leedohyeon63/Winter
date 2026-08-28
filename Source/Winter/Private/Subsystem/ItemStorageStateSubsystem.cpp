#include "Subsystem/ItemStorageStateSubsystem.h"

const FItemStorageRuntimeState* UItemStorageStateSubsystem::FindStorageState(
	const FName StorageStateKey) const
{
	return StorageStates.Find(StorageStateKey);
}

void UItemStorageStateSubsystem::SaveStorageState(
	const FName StorageStateKey,
	const TArray<FInventoryStack>& Contents)
{
	if (StorageStateKey.IsNone())
	{
		return;
	}

	FItemStorageRuntimeState& State = StorageStates.FindOrAdd(StorageStateKey);
	State.Contents = Contents;
}
