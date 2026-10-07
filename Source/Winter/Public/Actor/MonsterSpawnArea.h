#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DataAsset/MentalityWorldConfigDataAsset.h"
#include "MonsterSpawnArea.generated.h"

class ABaseMonster;
class UBoxComponent;
class UNavigationInvokerComponent;
class UMonsterPoolSubsystem;

struct FAreaMonsterInstance
{
	TWeakObjectPtr<ABaseMonster> Monster;
	uint32 LifeGeneration = 0;
};

/** 플레이어 스폰 범위와 겹치는 박스 안에서 지정한 몬스터만 생성한다. */
UCLASS(Blueprintable)
class WINTER_API AMonsterSpawnArea : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawnArea();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area")
	TObjectPtr<UBoxComponent> SpawnBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area")
	TSubclassOf<ABaseMonster> MonsterClass;

	/** 해당 단계의 클래스가 비어 있으면 기본 MonsterClass를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area|Mentality")
	TMap<EMentalityWorldState, TSubclassOf<ABaseMonster>> MentalityMonsterOverrides;

	UFUNCTION(BlueprintPure, Category = "Monster Spawn Area|Mentality")
	TSubclassOf<ABaseMonster> GetEffectiveMonsterClass() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawn Area")
	bool bSpawnEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area", meta = (ClampMin = "0"))
	int32 MaxMonsters = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area", meta = (ClampMin = "0.1", Units = "s"))
	float SpawnInterval = 2.0f;

	/** 플레이어와 몬스터 외곽 사이에 확보할 최소 수평 거리. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area|Spawn Safety", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumPlayerDistance = 1000.0f;

	/** 벽 뒤라도 카메라 시야 방향에 걸치면 보수적으로 생성하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area|Spawn Safety")
	bool bAvoidCameraView = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area|Spawn Safety", meta = (ClampMin = "0.0", ClampMax = "30.0", Units = "deg"))
	float ViewSafetyMargin = 10.0f;

	UFUNCTION(BlueprintPure, Category = "Monster Spawn Area")
	int32 GetActiveMonsterCount() const;

	bool IntersectsPlayerRange(const FVector& PlayerLocation, float Radius) const;
	bool IsSpawnLocationAllowed(const FVector& Location, const FVector& PlayerLocation, float Radius) const;
	void RefreshMonsters(const FVector& PlayerLocation, float DespawnRadius, UMonsterPoolSubsystem& Pool);
	void ReleaseAllMonsters(UMonsterPoolSubsystem& Pool);
	void UpdateNavigation(bool bNeeded);
	bool TrySpawnMonster(const FVector& PlayerLocation, float SpawnRadius, UMonsterPoolSubsystem& Pool);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster Spawn Area|Navigation")
	TObjectPtr<UNavigationInvokerComponent> NavigationInvoker;

	/** NavMesh가 준비되지 않았거나 지점이 막혀 있으면 다음 관리 주기에 다시 시도한다. */
	UPROPERTY(EditAnywhere, Category = "Monster Spawn Area|Navigation", meta = (ClampMin = "1", ClampMax = "64"))
	int32 SpawnLocationAttempts = 16;

private:
	bool IsOwnedInstanceActive(const FAreaMonsterInstance& Instance) const;
	bool FindSpawnTransform(const FVector& PlayerLocation, float SpawnRadius, FTransform& OutTransform) const;
	bool IsSpawnPresentationAllowed(const FVector& Location, const FVector& PlayerLocation, float BoundsRadius) const;
	TArray<FAreaMonsterInstance> SpawnedMonsters;
	TSubclassOf<ABaseMonster> ConfiguredPoolClass;
	double NextSpawnTime = 0.0;

	friend class FMonsterSpawnAreaLifecycleTest;
	friend class FMonsterSpawnSafetyTest;
};
