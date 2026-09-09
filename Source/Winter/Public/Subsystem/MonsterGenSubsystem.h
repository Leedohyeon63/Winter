#pragma once

#include "CoreMinimal.h"
#include "DataAsset/MentalityWorldConfigDataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "MonsterGenSubsystem.generated.h"

class AMonsterSpawnArea;
class AMainGameState;

/** 플레이어 범위 안의 배치된 스폰 구역을 관리한다. 멘탈리티 설정은 전체 수와 관리 주기를 결정한다. */
UCLASS(Blueprintable)
class WINTER_API UMonsterGenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	void RegisterSpawnArea(AMonsterSpawnArea* Area);
	void UnregisterSpawnArea(AMonsterSpawnArea* Area);

	UFUNCTION(BlueprintPure, Category = "Monster Spawn")
	float GetActiveSpawnRadius() const { return ActiveSpawnRadius; }

	UFUNCTION(BlueprintPure, Category = "Monster Spawn")
	float GetActiveDespawnRadius() const { return ActiveDespawnRadius; }

	UFUNCTION(BlueprintPure, Category = "Monster Spawn")
	int32 GetActiveMonsterCount() const;

private:
	UFUNCTION()
	void ManageMonsters();

	UFUNCTION()
	void HandleMentalityWorldStateChanged(EMentalityWorldState NewState);

	void ApplySpawnProfile(EMentalityWorldState NewState);
	void RestartManageTimer();

	/** 이전 DataAsset에 작은 반경이 저장돼 있어도 적용되는 최소 탐색 반경이다. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Areas", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumSpawnRadius = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Areas", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumDespawnRadius = 15000.0f;

	// 기존 BP 설정을 유지한다. 몬스터 종류와 허용 위치는 이제 배치한 구역이 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	int32 MaxMonsters = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float SpawnRadius = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float DespawnRadius = 15000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float CheckInterval = 1.0f;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AMonsterSpawnArea>> SpawnAreas;

	UPROPERTY(Transient)
	TObjectPtr<AMainGameState> CachedMainGameState;

	int32 ActiveMaxMonsters = 10;
	float ActiveSpawnRadius = 10000.0f;
	float ActiveDespawnRadius = 15000.0f;
	float ActiveCheckInterval = 1.0f;
	int32 NextAreaIndex = 0;
	bool bWorldStarted = false;
	FTimerHandle SpawnTimerHandle;
	friend class FMonsterSpawnAreaNavigationTest;
};
