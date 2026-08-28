#include "Subsystem/ItemSpawnStateSubsystem.h"

#include "HAL/PlatformTime.h"

void UItemSpawnStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// [아이템 스포너 추가] 게임을 새로 실행할 때마다 새로운 배치가 만들어지게 한다.
	SessionSeed = static_cast<int32>(FPlatformTime::Cycles());
}

const FItemSpawnerRuntimeState* UItemSpawnStateSubsystem::FindSpawnerState(
	const FName SpawnerStateKey) const
{
	return SpawnerStates.Find(SpawnerStateKey);
}

void UItemSpawnStateSubsystem::SaveSelection(
	const FName SpawnerStateKey,
	const int32 SelectedEntryIndex,
	const int32 Quantity,
	const float RandomYaw)
{
	if (SpawnerStateKey.IsNone())
	{
		return;
	}

	FItemSpawnerRuntimeState& State = SpawnerStates.FindOrAdd(SpawnerStateKey);
	State.bInitialized = true;
	State.bConsumed = false;
	State.SelectedEntryIndex = SelectedEntryIndex;
	State.RemainingQuantity = FMath::Max(0, Quantity);
	State.RandomYaw = RandomYaw;
}

void UItemSpawnStateSubsystem::UpdateRemainingQuantity(
	const FName SpawnerStateKey,
	const int32 RemainingQuantity)
{
	if (FItemSpawnerRuntimeState* State = SpawnerStates.Find(SpawnerStateKey))
	{
		State->RemainingQuantity = FMath::Max(0, RemainingQuantity);
	}
}

void UItemSpawnStateSubsystem::MarkConsumed(const FName SpawnerStateKey)
{
	if (SpawnerStateKey.IsNone())
	{
		return;
	}

	FItemSpawnerRuntimeState& State = SpawnerStates.FindOrAdd(SpawnerStateKey);
	State.bInitialized = true;
	State.bConsumed = true;
	State.RemainingQuantity = 0;
}

int32 UItemSpawnStateSubsystem::MakeRandomSeed(const FName SpawnerStateKey) const
{
	return static_cast<int32>(HashCombine(GetTypeHash(SpawnerStateKey), GetTypeHash(SessionSeed)));
}
