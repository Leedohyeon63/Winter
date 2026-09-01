#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MonsterPoolSubsystem.generated.h"

class ABaseMonster;

DECLARE_LOG_CATEGORY_EXTERN(LogMonsterPool, Log, All);

/** [몬스터 풀링 추가] 한 몬스터 클래스의 전체 개체와 비활성 대기 개체를 보관한다. */
USTRUCT()
struct WINTER_API FMonsterPoolBucket
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABaseMonster>> AllMonsters;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABaseMonster>> InactiveMonsters;

	int32 MaxRetainedSize = 16;
};

/**
 * [몬스터 풀링 추가]
 * 현재 월드의 BaseMonster를 클래스별로 미리 생성하고 거리 이탈 또는 사망 후 재사용한다.
 */
UCLASS()
class WINTER_API UMonsterPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void ConfigurePool(
		TSubclassOf<ABaseMonster> MonsterClass,
		int32 PrewarmCount,
		int32 MaxRetainedSize);

	ABaseMonster* AcquireMonster(
		TSubclassOf<ABaseMonster> MonsterClass,
		const FTransform& SpawnTransform);

	void ReleaseMonster(ABaseMonster* Monster);

	UFUNCTION(BlueprintPure, Category = "Monster|Pool")
	int32 GetActiveMonsterCount(TSubclassOf<ABaseMonster> MonsterClass) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Pool")
	int32 GetInactiveMonsterCount(TSubclassOf<ABaseMonster> MonsterClass) const;

private:
	ABaseMonster* CreateMonster(TSubclassOf<ABaseMonster> MonsterClass);
	void CleanupInvalidMonsters(FMonsterPoolBucket& Pool);

	// [몬스터 풀링 추가] 비활성 몬스터와 AIController가 GC되지 않도록 강한 참조로 보관한다.
	UPROPERTY(Transient)
	TMap<TSubclassOf<ABaseMonster>, FMonsterPoolBucket> MonsterPools;
};
