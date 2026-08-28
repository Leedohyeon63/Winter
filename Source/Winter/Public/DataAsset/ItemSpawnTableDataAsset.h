#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemSpawnTableDataAsset.generated.h"

class ASpawnerPickupActor;
class UItemDefinitionDataAsset;

/**
 * [아이템 스포너 추가]
 * 하나의 아이템 후보와 가중치, 생성 수량을 정의한다.
 */
USTRUCT(BlueprintType)
struct WINTER_API FWeightedItemSpawnEntry
{
	GENERATED_BODY()

	/** 생성할 아이템 데이터다. 비어 있는 항목은 추첨에서 제외된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UItemDefinitionDataAsset> ItemDefinition = nullptr;

	/**
	 * 이 항목에 사용할 Pickup 블루프린트 클래스다.
	 * 비어 있으면 스포너의 DefaultPickupActorClass를 사용한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<ASpawnerPickupActor> PickupActorClass;

	/** 다른 항목과 비교할 상대 가중치다. 0이면 생성되지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** 한 번에 생성할 최소 수량이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	/** 한 번에 생성할 최대 수량이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;
};

/**
 * [아이템 스포너 추가]
 * 여러 스포너가 공유할 수 있는 가중치 기반 아이템 생성표다.
 */
UCLASS(BlueprintType)
class WINTER_API UItemSpawnTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** 생성 가능한 아이템 목록이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Table")
	TArray<FWeightedItemSpawnEntry> Entries;

	/**
	 * 아무것도 생성하지 않을 가중치다.
	 * 0이면 유효한 아이템 중 하나가 반드시 선택된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Table", meta = (ClampMin = "0.0"))
	float EmptyWeight = 0.0f;
};
