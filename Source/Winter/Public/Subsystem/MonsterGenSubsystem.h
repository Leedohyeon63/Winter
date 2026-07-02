// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MonsterGenSubsystem.generated.h"

/**
 * 
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

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings|Filter")
	TArray<TSoftObjectPtr<UWorld>> MonsterSpawnLevelNames;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings")
	TArray<TSubclassOf<AActor>> MonsterClassesToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings")
	int32 MaxMonsters = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings")
	float SpawnRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings")
	float DespawnRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings")
	float CheckInterval = 1.0f;


	UPROPERTY()
	TArray<AActor*> ActiveMonsters;

	FTimerHandle SpawnTimerHandle;
};

