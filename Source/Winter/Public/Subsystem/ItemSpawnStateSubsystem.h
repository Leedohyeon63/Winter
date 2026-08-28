#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemSpawnStateSubsystem.generated.h"

/**
 * [아이템 스포너 추가]
 * 한 스포너가 현재 게임 실행 중 선택한 결과를 보관한다.
 * SelectedEntryIndex가 INDEX_NONE이면 '빈 결과'를 선택한 것이다.
 */
struct FItemSpawnerRuntimeState
{
	bool bInitialized = false;
	bool bConsumed = false;
	int32 SelectedEntryIndex = INDEX_NONE;
	int32 RemainingQuantity = 0;
	float RandomYaw = 0.0f;
};

/**
 * [아이템 스포너 추가]
 * OpenLevel 이후에도 살아 있는 GameInstance Subsystem에서 스포너 상태를 관리한다.
 * 아이템을 전부 주운 스포너는 같은 게임 실행 중 다시 생성되지 않는다.
 */
UCLASS()
class WINTER_API UItemSpawnStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 이미 추첨된 스포너의 상태를 반환한다. */
	const FItemSpawnerRuntimeState* FindSpawnerState(FName SpawnerStateKey) const;

	/** 최초 추첨 결과를 저장한다. */
	void SaveSelection(
		FName SpawnerStateKey,
		int32 SelectedEntryIndex,
		int32 Quantity,
		float RandomYaw);

	/** 일부만 주운 경우 남은 수량을 저장한다. */
	void UpdateRemainingQuantity(FName SpawnerStateKey, int32 RemainingQuantity);

	/** 아이템을 모두 주운 스포너를 영구 빈 상태로 표시한다. */
	void MarkConsumed(FName SpawnerStateKey);

	/** 현재 실행마다 달라지면서 같은 스포너에는 안정적인 랜덤 시드를 만든다. */
	int32 MakeRandomSeed(FName SpawnerStateKey) const;

private:
	TMap<FName, FItemSpawnerRuntimeState> SpawnerStates;
	int32 SessionSeed = 0;
};
