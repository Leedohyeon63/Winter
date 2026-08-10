#include "Subsystem/MonsterGenSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameState/MainGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"
#include "NavigationSystem.h"
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
				ActiveMonsterClasses.Add(TSubclassOf<AActor>(MonsterClass.Get()));
			}
		}

		ActiveMaxMonsters = FMath::Max(0, Profile->MonsterSpawn.MaxMonsters);
		ActiveSpawnRadius = FMath::Max(0.0f, Profile->MonsterSpawn.SpawnRadius);
		ActiveDespawnRadius = FMath::Max(
			ActiveSpawnRadius,
			Profile->MonsterSpawn.DespawnRadius);
		ActiveCheckInterval = FMath::Max(0.1f, Profile->MonsterSpawn.CheckInterval);
		return;
	}

	// [호환 유지] 설정 에셋 또는 현재 상태 프로필이 없으면 기존 값을 복사한다.
	ActiveMonsterClasses = MonsterClassesToSpawn;
	ActiveMaxMonsters = FMath::Max(0, MaxMonsters);
	ActiveSpawnRadius = FMath::Max(0.0f, SpawnRadius);
	ActiveDespawnRadius = FMath::Max(ActiveSpawnRadius, DespawnRadius);
	ActiveCheckInterval = FMath::Max(0.1f, CheckInterval);
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

	for (int32 Index = ActiveMonsters.Num() - 1; Index >= 0; --Index)
	{
		AActor* Monster = ActiveMonsters[Index];

		if (!IsValid(Monster))
		{
			ActiveMonsters.RemoveAt(Index);
			continue;
		}

		const float DistanceToPlayer =
			FVector::Dist(PlayerLocation, Monster->GetActorLocation());

		if (DistanceToPlayer > ActiveDespawnRadius)
		{
			Monster->Destroy();
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
	const TSubclassOf<AActor> SelectedMonsterClass = ActiveMonsterClasses[RandomIndex];

	if (!SelectedMonsterClass)
	{
		return;
	}

	AActor* SpawnedMonster = World->SpawnActor<AActor>(
		SelectedMonsterClass,
		RandomNavLocation.Location,
		FRotator::ZeroRotator);

	if (SpawnedMonster)
	{
		ActiveMonsters.Add(SpawnedMonster);
	}
}
