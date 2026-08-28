#pragma once

#include "CoreMinimal.h"
#include "Struct/InventoryTypeStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemStorageStateSubsystem.generated.h"


USTRUCT()
struct WINTER_API FItemStorageRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryStack> Contents;
};


UCLASS()
class WINTER_API UItemStorageStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	const FItemStorageRuntimeState* FindStorageState(FName StorageStateKey) const;
	void SaveStorageState(FName StorageStateKey, const TArray<FInventoryStack>& Contents);

private:
	UPROPERTY()
	TMap<FName, FItemStorageRuntimeState> StorageStates;
};
