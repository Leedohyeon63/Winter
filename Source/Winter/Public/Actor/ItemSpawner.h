#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawner.generated.h"

class ASpawnerPickupActor;
class UArrowComponent;
class UItemSpawnStateSubsystem;
class UItemSpawnTableDataAsset;
class USceneComponent;

/**
 * [아이템 스포너 추가]
 * 데이터 에셋의 가중치에 따라 하나의 Pickup을 생성한다.
 * 추첨 결과와 획득 여부는 GameInstance Subsystem에 보존된다.
 */
UCLASS()
class WINTER_API AItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	AItemSpawner();

	/** BeginPlay 외에 테스트 목적으로 수동 호출할 수 있다. */
	UFUNCTION(BlueprintCallable, Category = "Item Spawner")
	bool SpawnItem();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> SpawnDirection;

	/**
	 * 아이템 종류, 수량, 가중치를 보관하는 데이터 에셋이다.
	 * EditAnywhere이므로 C++ 클래스의 자식 블루프린트 기본값에도 미리 지정할 수 있다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner")
	TObjectPtr<UItemSpawnTableDataAsset> SpawnTable;

	/** 항목별 PickupActorClass가 비어 있을 때 사용할 기본 클래스다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner")
	TSubclassOf<ASpawnerPickupActor> DefaultPickupActorClass;

	/**
	 * 맵 안에서 중복되지 않는 ID다. 비어 있으면 배치된 액터 이름을 사용한다.
	 * 액터 이름을 바꾸더라도 상태를 유지하려면 명시적으로 입력하는 것이 안전하다.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Item Spawner|Persistence")
	FName SpawnerId = NAME_None;

	/** 스포너 위치를 기준으로 적용할 생성 오프셋이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner|Transform")
	FVector SpawnOffset = FVector::ZeroVector;

	/** 스포너 회전에 더할 생성 회전이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner|Transform")
	FRotator SpawnRotation = FRotator::ZeroRotator;

	/** 같은 방향으로만 놓이지 않도록 최초 추첨 시 Yaw를 무작위로 정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner|Transform")
	bool bRandomYaw = true;

	/** 테스트 중 항상 같은 결과를 확인하고 싶을 때 활성화한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawner|Debug")
	bool bUseFixedSeed = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Item Spawner|Debug",
		meta = (EditCondition = "bUseFixedSeed"))
	int32 FixedSeed = 12345;

private:
	FName BuildSpawnerStateKey() const;
	int32 SelectWeightedEntry(FRandomStream& RandomStream) const;
	bool SpawnSelectedEntry(
		int32 EntryIndex,
		int32 Quantity,
		float RandomYaw,
		FName SpawnerStateKey);

	UPROPERTY(Transient)
	TObjectPtr<ASpawnerPickupActor> SpawnedPickup;
};
