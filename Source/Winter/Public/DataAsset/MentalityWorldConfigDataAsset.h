#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MentalityWorldConfigDataAsset.generated.h"

class ABaseMonster;
class UWorld;

UENUM(BlueprintType)
enum class EMentalityWorldState : uint8
{
	Stable UMETA(DisplayName = "Stable"),
	Uneasy UMETA(DisplayName = "Uneasy"),
	Distorted UMETA(DisplayName = "Distorted"),
	Critical UMETA(DisplayName = "Critical")
};

USTRUCT(BlueprintType)
struct WINTER_API FMentalityMonsterSpawnSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TArray<TSubclassOf<ABaseMonster>> MonsterClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
	int32 MaxMonsters = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float DespawnRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.1"))
	float CheckInterval = 1.0f;
};


USTRUCT(BlueprintType)
struct WINTER_API FMentalityWorldStateProfile
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mentality")
	EMentalityWorldState State = EMentalityWorldState::Stable;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Mentality",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MinimumMentalityPercent = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mentality")
	FMentalityMonsterSpawnSettings MonsterSpawn;
};


UCLASS(BlueprintType)
class WINTER_API UMentalityWorldConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TArray<TSoftObjectPtr<UWorld>> MonsterSpawnLevels;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mentality")
	TArray<FMentalityWorldStateProfile> StateProfiles;

	const FMentalityWorldStateProfile* FindProfileByState(EMentalityWorldState State) const;
	const FMentalityWorldStateProfile* FindProfileForPercent(float MentalityPercent) const;
};
