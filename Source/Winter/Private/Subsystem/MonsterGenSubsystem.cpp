#include "Subsystem/MonsterGenSubsystem.h"

#include "Actor/MonsterSpawnArea.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameState/MainGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"
#include "Subsystem/MonsterPoolSubsystem.h"
#include "TimerManager.h"

void UMonsterGenSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bWorldStarted = true;
	CachedMainGameState = InWorld.GetGameState<AMainGameState>();
	if (CachedMainGameState)
	{
		CachedMainGameState->OnMentalityWorldStateChanged.AddDynamic(
			this, &UMonsterGenSubsystem::HandleMentalityWorldStateChanged);
	}
	ApplySpawnProfile(CachedMainGameState ? CachedMainGameState->CurrentMentalityWorldState : EMentalityWorldState::Stable);
	RestartManageTimer();
}

void UMonsterGenSubsystem::Deinitialize()
{
	bWorldStarted = false;
	if (IsValid(CachedMainGameState))
	{
		CachedMainGameState->OnMentalityWorldStateChanged.RemoveDynamic(
			this, &UMonsterGenSubsystem::HandleMentalityWorldStateChanged);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
	SpawnAreas.Reset();
	CachedMainGameState = nullptr;
	Super::Deinitialize();
}

void UMonsterGenSubsystem::RegisterSpawnArea(AMonsterSpawnArea* Area)
{
	if (IsValid(Area) && Area->GetWorld() == GetWorld())
	{
		SpawnAreas.AddUnique(Area);
		RestartManageTimer();
	}
}

void UMonsterGenSubsystem::UnregisterSpawnArea(AMonsterSpawnArea* Area)
{
	SpawnAreas.RemoveAll([Area](const TWeakObjectPtr<AMonsterSpawnArea>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Area;
	});
	RestartManageTimer();
}

void UMonsterGenSubsystem::HandleMentalityWorldStateChanged(EMentalityWorldState NewState)
{
	ApplySpawnProfile(NewState);
	RestartManageTimer();
}

void UMonsterGenSubsystem::ApplySpawnProfile(EMentalityWorldState NewState)
{
	const UMentalityWorldConfigDataAsset* Config = CachedMainGameState ? CachedMainGameState->GetMentalityWorldConfig() : nullptr;
	const FMentalityWorldStateProfile* Profile = Config ? Config->FindProfileByState(NewState) : nullptr;
	ActiveMaxMonsters = FMath::Max(0, Profile ? Profile->MonsterSpawn.MaxMonsters : MaxMonsters);
	ActiveSpawnRadius = FMath::Max(FMath::Max(0.0f, MinimumSpawnRadius), Profile ? Profile->MonsterSpawn.SpawnRadius : SpawnRadius);
	ActiveDespawnRadius = FMath::Max(ActiveSpawnRadius, FMath::Max(MinimumDespawnRadius,
		Profile ? Profile->MonsterSpawn.DespawnRadius : DespawnRadius));
	ActiveCheckInterval = FMath::Max(0.1f, Profile ? Profile->MonsterSpawn.CheckInterval : CheckInterval);
}

void UMonsterGenSubsystem::RestartManageTimer()
{
	UWorld* World = GetWorld();
	if (!World || !bWorldStarted)
	{
		return;
	}
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	if (!SpawnAreas.IsEmpty())
	{
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this,
			&UMonsterGenSubsystem::ManageMonsters, ActiveCheckInterval, true);
	}
}

int32 UMonsterGenSubsystem::GetActiveMonsterCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AMonsterSpawnArea>& Area : SpawnAreas)
	{
		if (Area.IsValid())
		{
			Count += Area->GetActiveMonsterCount();
		}
	}
	return Count;
}

void UMonsterGenSubsystem::ManageMonsters()
{
	UWorld* World = GetWorld();
	UMonsterPoolSubsystem* Pool = World ? World->GetSubsystem<UMonsterPoolSubsystem>() : nullptr;
	if (!World || !Pool || World->bIsTearingDown)
	{
		return;
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	// 반환 이벤트가 구역을 제거해도 순회 중인 배열이 바뀌지 않도록 스냅샷을 사용한다.
	const TArray<TWeakObjectPtr<AMonsterSpawnArea>> Areas = SpawnAreas;
	for (const TWeakObjectPtr<AMonsterSpawnArea>& Entry : Areas)
	{
		AMonsterSpawnArea* Area = Entry.Get();
		if (!IsValid(Area))
		{
			continue;
		}
		if (Player)
		{
			Area->RefreshMonsters(Player->GetActorLocation(), ActiveDespawnRadius, *Pool);
		}
		else
		{
			Area->ReleaseAllMonsters(*Pool);
		}
		if (IsValid(Area))
		{
			const bool bInRange = Player && Area->bSpawnEnabled && Area->MonsterClass && Area->MaxMonsters > 0
				&& ActiveMaxMonsters > 0 && Area->IntersectsPlayerRange(Player->GetActorLocation(), ActiveSpawnRadius);
			Area->UpdateNavigation(bInRange || Area->GetActiveMonsterCount() > 0);
		}
	}
	if (!Player || Areas.IsEmpty() || GetActiveMonsterCount() >= ActiveMaxMonsters)
	{
		return;
	}
	// 구역을 돌아가며 시도해 먼저 등록한 구역이 전체 한도를 독점하지 않도록 한다.
	const int32 StartIndex = NextAreaIndex % Areas.Num();
	for (int32 Offset = 0; Offset < Areas.Num(); ++Offset)
	{
		const int32 Index = (StartIndex + Offset) % Areas.Num();
		AMonsterSpawnArea* Area = Areas[Index].Get();
		if (IsValid(Area) && Area->TrySpawnMonster(Player->GetActorLocation(), ActiveSpawnRadius, *Pool))
		{
			NextAreaIndex = (Index + 1) % Areas.Num();
			return;
		}
	}
	NextAreaIndex = (StartIndex + 1) % Areas.Num();
}
