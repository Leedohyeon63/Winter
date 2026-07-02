// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/MonsterGenSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UMonsterGenSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UE_LOG(LogTemp, Warning, TEXT("[WinterDebug] >>> 서브시스템 가동됨! <<<"));
	FString CurrentMapName = InWorld.GetMapName();
	CurrentMapName.RemoveFromStart(InWorld.StreamingLevelsPrefix);

	bool bIsAllowed = false;

	for (const TSoftObjectPtr<UWorld>& LevelAsset : MonsterSpawnLevelNames)
	{
		if (LevelAsset.IsNull()) continue;

		if (LevelAsset.GetAssetName() == CurrentMapName)
		{
			bIsAllowed = true;
			break;
		}
	}

	if (!bIsAllowed)
	{
		return;
	}

	InWorld.GetTimerManager().SetTimer(SpawnTimerHandle, this, &UMonsterGenSubsystem::ManageMonsters, CheckInterval, true);

}

void UMonsterGenSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	Super::Deinitialize();
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

	FVector PlayerLocation = Player->GetActorLocation();

	for (int32 i = ActiveMonsters.Num() - 1; i >= 0; --i)
	{
		AActor* Monster = ActiveMonsters[i];

		if (!IsValid(Monster))
		{
			ActiveMonsters.RemoveAt(i);
			continue;
		}

		float DistanceToPlayer = FVector::Dist(PlayerLocation, Monster->GetActorLocation());
		if (DistanceToPlayer > DespawnRadius)
		{
			Monster->Destroy();
			ActiveMonsters.RemoveAt(i);
		}
	}

	if (ActiveMonsters.Num() < MaxMonsters && MonsterClassesToSpawn.Num() > 0)
	{
		UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
		FNavLocation RandomNavLocation;

		if (NavSystem && NavSystem->GetRandomReachablePointInRadius(PlayerLocation, SpawnRadius, RandomNavLocation))
		{
			int32 RandomIndex = FMath::RandRange(0, MonsterClassesToSpawn.Num() - 1);

			TSubclassOf<AActor> SelectedMonsterClass = MonsterClassesToSpawn[RandomIndex];

			if (SelectedMonsterClass != nullptr)
			{
				AActor* SpawnedMonster = World->SpawnActor<AActor>(SelectedMonsterClass, RandomNavLocation.Location, FRotator::ZeroRotator);

				if (SpawnedMonster)
				{
					ActiveMonsters.Add(SpawnedMonster);
					UE_LOG(LogTemp, Log, TEXT("몬스터 스폰"));
				}
			}
		}
	}
}