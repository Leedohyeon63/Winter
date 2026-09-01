#pragma once

#include "CoreMinimal.h"
#include "DataAsset/MentalityWorldConfigDataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "MonsterGenSubsystem.generated.h"

class AActor;
class ABaseMonster;
class AMainGameState;
class UWorld;

/**
 * 허용된 레벨에서 플레이어 주변 몬스터를 관리한다.
 * MainGameState의 멘탈리티 월드 상태를 구독해 상태별 스폰 규칙을 런타임에 교체한다.
 */
UCLASS(Blueprintable)
class WINTER_API UMonsterGenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	UFUNCTION()
	void ManageMonsters();

	// [멘탈리티 월드 상태 추가] 중앙 상태 변경 이벤트를 받아 현재 스폰 프로필을 교체한다.
	UFUNCTION()
	void HandleMentalityWorldStateChanged(EMentalityWorldState NewState);

	bool IsMonsterSpawnLevel(const UWorld& InWorld) const;
	void ApplySpawnProfile(EMentalityWorldState NewState);
	void RestartManageTimer();
	void ConfigureActiveMonsterPools();

	// [호환 유지] MentalityWorldConfig에 레벨 목록이 없으면 기존 필터를 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	TArray<TSoftObjectPtr<UWorld>> MonsterSpawnLevelNames;

	// [호환 유지] 설정 에셋이 없을 때 기존 고정 스폰 목록을 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	TArray<TSubclassOf<AActor>> MonsterClassesToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	int32 MaxMonsters = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float SpawnRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float DespawnRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Legacy")
	float CheckInterval = 1.0f;

	// [몬스터 풀링 추가] 현재 프로필의 각 몬스터 클래스에 미리 생성할 개체 수다.
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Pool", meta = (ClampMin = "0"))
	int32 MonsterPoolPrewarmCountPerClass = 4;

	// [몬스터 풀링 추가] 거리 이탈과 사망 후 클래스별로 계속 보관할 최대 개체 수다.
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Pool", meta = (ClampMin = "1"))
	int32 MonsterPoolMaxRetainedSizePerClass = 16;

	// [멘탈리티 월드 상태 추가] 현재 프로필에서 복사한 런타임 스폰 목록이다.
	UPROPERTY(Transient)
	TArray<TSubclassOf<ABaseMonster>> ActiveMonsterClasses;

	UPROPERTY(Transient)
	int32 ActiveMaxMonsters = 10;

	UPROPERTY(Transient)
	float ActiveSpawnRadius = 1500.0f;

	UPROPERTY(Transient)
	float ActiveDespawnRadius = 3000.0f;

	UPROPERTY(Transient)
	float ActiveCheckInterval = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<AMainGameState> CachedMainGameState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABaseMonster>> ActiveMonsters;

	bool bSpawnEnabledForCurrentWorld = false;
	FTimerHandle SpawnTimerHandle;
};
