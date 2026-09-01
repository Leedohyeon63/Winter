#include "Subsystem/MonsterGenSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameState/MainGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"
#include "NavigationSystem.h"
#include "Subsystem/MonsterPoolSubsystem.h"
#include "TimerManager.h"

void UMonsterGenSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedMainGameState = InWorld.GetGameState<AMainGameState>();
	bSpawnEnabledForCurrentWorld = IsMonsterSpawnLevel(InWorld);

	if (!bSpawnEnabledForCurrentWorld)
	{
		return;
	}

	if (CachedMainGameState)
	{
		// [멘탈리티 월드 상태 추가] 중앙 계층의 상태 변경을 구독해 독립적으로 스폰 규칙을 갱신한다.
		CachedMainGameState->OnMentalityWorldStateChanged.AddDynamic(
			this,
			&UMonsterGenSubsystem::HandleMentalityWorldStateChanged);

		ApplySpawnProfile(CachedMainGameState->CurrentMentalityWorldState);
	}
	else
	{
		// [호환 유지] 커스텀 MainGameState가 아닌 레벨에서도 기존 설정으로 동작한다.
		ApplySpawnProfile(EMentalityWorldState::Stable);
	}

	RestartManageTimer();
}

void UMonsterGenSubsystem::Deinitialize()
{
	if (CachedMainGameState)
	{
		CachedMainGameState->OnMentalityWorldStateChanged.RemoveDynamic(
			this,
			&UMonsterGenSubsystem::HandleMentalityWorldStateChanged);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	ActiveMonsters.Reset();
	ActiveMonsterClasses.Reset();
	CachedMainGameState = nullptr;

	Super::Deinitialize();
}

void UMonsterGenSubsystem::HandleMentalityWorldStateChanged(
	EMentalityWorldState NewState)
{
	ApplySpawnProfile(NewState);
	RestartManageTimer();
}

bool UMonsterGenSubsystem::IsMonsterSpawnLevel(const UWorld& InWorld) const
{
	const UMentalityWorldConfigDataAsset* Config = CachedMainGameState
		? CachedMainGameState->GetMentalityWorldConfig()
		: nullptr;

	const TArray<TSoftObjectPtr<UWorld>>& LevelsToCheck =
		Config && Config->MonsterSpawnLevels.Num() > 0
		? Config->MonsterSpawnLevels
		: MonsterSpawnLevelNames;

	FString CurrentMapName = InWorld.GetMapName();
	CurrentMapName.RemoveFromStart(InWorld.StreamingLevelsPrefix);

	for (const TSoftObjectPtr<UWorld>& LevelAsset : LevelsToCheck)
	{
		if (!LevelAsset.IsNull() && LevelAsset.GetAssetName() == CurrentMapName)
		{
			return true;
		}
	}

	return false;
}

void UMonsterGenSubsystem::ApplySpawnProfile(EMentalityWorldState NewState)
{
	ActiveMonsterClasses.Reset();

	const UMentalityWorldConfigDataAsset* Config = CachedMainGameState
		? CachedMainGameState->GetMentalityWorldConfig()
		: nullptr;

	const FMentalityWorldStateProfile* Profile = Config
		? Config->FindProfileByState(NewState)
		: nullptr;

	if (Profile)
	{
		// [멘탈리티 월드 상태 추가] 선택된 단계의 몬스터와 수량/거리/주기를 런타임 설정으로 적용한다.
		for (const TSubclassOf<ABaseMonster>& MonsterClass : Profile->MonsterSpawn.MonsterClasses)
		{
			if (MonsterClass)
			{
				ActiveMonsterClasses.Add(MonsterClass);
			}
		}

		ActiveMaxMonsters = FMath::Max(0, Profile->MonsterSpawn.MaxMonsters);
		ActiveSpawnRadius = FMath::Max(0.0f, Profile->MonsterSpawn.SpawnRadius);
		ActiveDespawnRadius = FMath::Max(
			ActiveSpawnRadius,
			Profile->MonsterSpawn.DespawnRadius);
		ActiveCheckInterval = FMath::Max(0.1f, Profile->MonsterSpawn.CheckInterval);
		ConfigureActiveMonsterPools();
		return;
	}

	// [호환 유지] 설정 에셋 또는 현재 상태 프로필이 없으면 기존 값을 복사한다.
	for (const TSubclassOf<AActor>& LegacyMonsterClass : MonsterClassesToSpawn)
	{
		if (LegacyMonsterClass
			&& LegacyMonsterClass->IsChildOf(ABaseMonster::StaticClass()))
		{
			// [몬스터 풀링 추가] 기존 배열은 유지하되 실제 풀에는 BaseMonster 자식만 등록한다.
			ActiveMonsterClasses.Add(
				TSubclassOf<ABaseMonster>(LegacyMonsterClass.Get()));
		}
		else if (LegacyMonsterClass)
		{
			UE_LOG(
				LogMonsterPool,
				Warning,
				TEXT("Skipped non-BaseMonster legacy class: %s"),
				*GetNameSafe(LegacyMonsterClass.Get()));
		}
	}
	ActiveMaxMonsters = FMath::Max(0, MaxMonsters);
	ActiveSpawnRadius = FMath::Max(0.0f, SpawnRadius);
	ActiveDespawnRadius = FMath::Max(ActiveSpawnRadius, DespawnRadius);
	ActiveCheckInterval = FMath::Max(0.1f, CheckInterval);
	ConfigureActiveMonsterPools();
}

void UMonsterGenSubsystem::ConfigureActiveMonsterPools()
{
	UWorld* World = GetWorld();
	UMonsterPoolSubsystem* MonsterPool = World
		? World->GetSubsystem<UMonsterPoolSubsystem>()
		: nullptr;
	if (!MonsterPool)
	{
		return;
	}

	for (const TSubclassOf<ABaseMonster>& MonsterClass : ActiveMonsterClasses)
	{
		if (MonsterClass)
		{
			// [몬스터 풀링 추가] 멘탈리티 프로필이 바뀌면 새 프로필의 클래스 풀도 즉시 예열한다.
			MonsterPool->ConfigurePool(
				MonsterClass,
				FMath::Max(0, MonsterPoolPrewarmCountPerClass),
				FMath::Max(1, MonsterPoolMaxRetainedSizePerClass));
		}
	}
}

void UMonsterGenSubsystem::RestartManageTimer()
{
	UWorld* World = GetWorld();
	if (!World || !bSpawnEnabledForCurrentWorld)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	World->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&UMonsterGenSubsystem::ManageMonsters,
		ActiveCheckInterval,
		true);
}

void UMonsterGenSubsystem::ManageMonsters()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player)
	{
		return;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	UMonsterPoolSubsystem* MonsterPool = World->GetSubsystem<UMonsterPoolSubsystem>();
	if (!MonsterPool)
	{
		return;
	}

	for (int32 Index = ActiveMonsters.Num() - 1; Index >= 0; --Index)
	{
		ABaseMonster* Monster = ActiveMonsters[Index];

		if (!IsValid(Monster) || !Monster->IsActiveMonster())
		{
			// [몬스터 풀링 추가] 사망 지연 후 이미 풀에 들어간 몬스터는 활성 목록에서 제외한다.
			ActiveMonsters.RemoveAt(Index);
			continue;
		}

		const float DistanceToPlayer =
			FVector::Dist(PlayerLocation, Monster->GetActorLocation());

		if (DistanceToPlayer > ActiveDespawnRadius)
		{
			// [몬스터 풀링 추가] 플레이어에게서 멀어진 몬스터는 제거하지 않고 풀로 반환한다.
			MonsterPool->ReleaseMonster(Monster);
			ActiveMonsters.RemoveAt(Index);
		}
	}

	if (ActiveMonsters.Num() >= ActiveMaxMonsters || ActiveMonsterClasses.IsEmpty())
	{
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
	FNavLocation RandomNavLocation;

	if (!NavSystem
		|| !NavSystem->GetRandomReachablePointInRadius(
			PlayerLocation,
			ActiveSpawnRadius,
			RandomNavLocation))
	{
		return;
	}

	const int32 RandomIndex = FMath::RandRange(0, ActiveMonsterClasses.Num() - 1);
	const TSubclassOf<ABaseMonster> SelectedMonsterClass = ActiveMonsterClasses[RandomIndex];

	if (!SelectedMonsterClass)
	{
		return;
	}

	// [몬스터 풀링 추가] SpawnActor 대신 클래스별 풀에서 몬스터를 가져와 체력과 AI를 초기화한다.
	ABaseMonster* SpawnedMonster = MonsterPool->AcquireMonster(
		SelectedMonsterClass,
		FTransform(FRotator::ZeroRotator, RandomNavLocation.Location));

	if (SpawnedMonster)
	{
		ActiveMonsters.Add(SpawnedMonster);
	}
}
