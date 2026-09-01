#include "Subsystem/MonsterPoolSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"

// [몬스터 풀링 추가] 생성과 재사용 여부를 Output Log에서 독립적으로 확인한다.
DEFINE_LOG_CATEGORY(LogMonsterPool);

void UMonsterPoolSubsystem::Deinitialize()
{
	for (TPair<TSubclassOf<ABaseMonster>, FMonsterPoolBucket>& Pair : MonsterPools)
	{
		for (ABaseMonster* Monster : Pair.Value.AllMonsters)
		{
			if (IsValid(Monster))
			{
				Monster->AssignToPool(nullptr);
			}
		}
	}

	MonsterPools.Empty();
	Super::Deinitialize();
}

void UMonsterPoolSubsystem::ConfigurePool(
	const TSubclassOf<ABaseMonster> MonsterClass,
	const int32 PrewarmCount,
	const int32 MaxRetainedSize)
{
	if (!MonsterClass || !GetWorld())
	{
		return;
	}

	FMonsterPoolBucket& Pool = MonsterPools.FindOrAdd(MonsterClass);
	CleanupInvalidMonsters(Pool);
	Pool.MaxRetainedSize = FMath::Max(1, MaxRetainedSize);

	const int32 SafePrewarmCount = FMath::Clamp(PrewarmCount, 0, Pool.MaxRetainedSize);
	while (Pool.AllMonsters.Num() < SafePrewarmCount)
	{
		ABaseMonster* Monster = CreateMonster(MonsterClass);
		if (!Monster)
		{
			break;
		}

		Pool.AllMonsters.Add(Monster);
		Pool.InactiveMonsters.Add(Monster);
	}

	// [몬스터 풀링 추가] 풀 설정을 낮춰도 활성 몬스터는 제거하지 않고 비활성 개체부터 정리한다.
	while (Pool.AllMonsters.Num() > Pool.MaxRetainedSize
		&& !Pool.InactiveMonsters.IsEmpty())
	{
		ABaseMonster* ExcessMonster = Pool.InactiveMonsters.Pop(EAllowShrinking::No);
		Pool.AllMonsters.RemoveSingleSwap(ExcessMonster);
		if (IsValid(ExcessMonster))
		{
			ExcessMonster->DestroyPermanentlyFromPool();
		}
	}
}

ABaseMonster* UMonsterPoolSubsystem::AcquireMonster(
	const TSubclassOf<ABaseMonster> MonsterClass,
	const FTransform& SpawnTransform)
{
	if (!MonsterClass || !GetWorld())
	{
		return nullptr;
	}

	FMonsterPoolBucket& Pool = MonsterPools.FindOrAdd(MonsterClass);
	CleanupInvalidMonsters(Pool);

	ABaseMonster* Monster = nullptr;
	while (!Pool.InactiveMonsters.IsEmpty() && !IsValid(Monster))
	{
		Monster = Pool.InactiveMonsters.Pop(EAllowShrinking::No);
	}
	const bool bReusedMonster = IsValid(Monster);

	if (!Monster)
	{
		// [몬스터 풀링 추가] 활성 수가 예열 수를 넘어도 스폰을 누락하지 않고 풀을 임시 확장한다.
		Monster = CreateMonster(MonsterClass);
		if (!Monster)
		{
			return nullptr;
		}
		Pool.AllMonsters.Add(Monster);
	}

	Monster->ActivateFromPool(SpawnTransform);
	UE_LOG(
		LogMonsterPool,
		Verbose,
		TEXT("Acquire Class=%s Reused=%s Active=%d Inactive=%d"),
		*GetNameSafe(MonsterClass.Get()),
		bReusedMonster ? TEXT("true") : TEXT("false"),
		FMath::Max(0, Pool.AllMonsters.Num() - Pool.InactiveMonsters.Num()),
		Pool.InactiveMonsters.Num());
	return Monster;
}

void UMonsterPoolSubsystem::ReleaseMonster(ABaseMonster* Monster)
{
	if (!IsValid(Monster))
	{
		return;
	}

	const TSubclassOf<ABaseMonster> MonsterClass = Monster->GetClass();
	FMonsterPoolBucket* Pool = MonsterPools.Find(MonsterClass);
	if (!Pool || !Pool->AllMonsters.Contains(Monster))
	{
		Monster->DestroyPermanentlyFromPool();
		return;
	}

	if (Pool->InactiveMonsters.Contains(Monster))
	{
		return;
	}

	Monster->DeactivateToPool();
	if (!IsValid(Monster))
	{
		// [몬스터 풀링 추가] Blueprint 반환 이벤트에서 제거된 예외적인 개체는 풀 목록에도 남기지 않는다.
		Pool->AllMonsters.RemoveSingleSwap(Monster);
		return;
	}

	if (Pool->AllMonsters.Num() > Pool->MaxRetainedSize)
	{
		Pool->AllMonsters.RemoveSingleSwap(Monster);
		UE_LOG(
			LogMonsterPool,
			Log,
			TEXT("Trimmed Class=%s Retained=%d"),
			*GetNameSafe(MonsterClass.Get()),
			Pool->AllMonsters.Num());
		Monster->DestroyPermanentlyFromPool();
		return;
	}

	Pool->InactiveMonsters.Add(Monster);
}

int32 UMonsterPoolSubsystem::GetActiveMonsterCount(
	const TSubclassOf<ABaseMonster> MonsterClass) const
{
	if (const FMonsterPoolBucket* Pool = MonsterPools.Find(MonsterClass))
	{
		return FMath::Max(0, Pool->AllMonsters.Num() - Pool->InactiveMonsters.Num());
	}
	return 0;
}

int32 UMonsterPoolSubsystem::GetInactiveMonsterCount(
	const TSubclassOf<ABaseMonster> MonsterClass) const
{
	if (const FMonsterPoolBucket* Pool = MonsterPools.Find(MonsterClass))
	{
		return Pool->InactiveMonsters.Num();
	}
	return 0;
}

ABaseMonster* UMonsterPoolSubsystem::CreateMonster(
	const TSubclassOf<ABaseMonster> MonsterClass)
{
	UWorld* World = GetWorld();
	if (!World || !MonsterClass)
	{
		return nullptr;
	}

	const FTransform StorageTransform(
		FRotator::ZeroRotator,
		FVector(0.0f, 0.0f, -100000.0f));

	// [몬스터 풀링 추가] AutoPossessAI가 실행되기 전에 비활성 상태를 주입하기 위해 지연 생성한다.
	ABaseMonster* DeferredMonster = World->SpawnActorDeferred<ABaseMonster>(
		MonsterClass,
		StorageTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!DeferredMonster)
	{
		return nullptr;
	}

	DeferredMonster->AssignToPool(this);
	ABaseMonster* Monster = Cast<ABaseMonster>(
		UGameplayStatics::FinishSpawningActor(DeferredMonster, StorageTransform));
	if (Monster)
	{
		// [몬스터 풀링 추가] 최초 예열은 실제 반환이 아니므로 Blueprint 반환 이벤트 없이 비활성화한다.
		Monster->DeactivateToPool(false);
		if (!IsValid(Monster))
		{
			return nullptr;
		}

		UE_LOG(
			LogMonsterPool,
			Log,
			TEXT("Created Class=%s Actor=%s"),
			*GetNameSafe(MonsterClass.Get()),
			*GetNameSafe(Monster));
	}
	return Monster;
}

void UMonsterPoolSubsystem::CleanupInvalidMonsters(FMonsterPoolBucket& Pool)
{
	Pool.AllMonsters.RemoveAll([](const TObjectPtr<ABaseMonster>& Monster)
	{
		return !IsValid(Monster);
	});

	Pool.InactiveMonsters.RemoveAll([](const TObjectPtr<ABaseMonster>& Monster)
	{
		return !IsValid(Monster);
	});
}
