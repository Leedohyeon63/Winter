#pragma once

#include "CoreMinimal.h"
#include "Actor/PickupActor.h"
#include "SpawnerPickupActor.generated.h"

class UItemDefinitionDataAsset;

/**
 * [아이템 스포너 추가]
 * APickupActor의 기존 상호작용을 그대로 사용하면서 스포너에 남은 수량과 획득 완료를 알린다.
 */
UCLASS()
class WINTER_API ASpawnerPickupActor : public APickupActor
{
	GENERATED_BODY()

public:
	/** SpawnActorDeferred와 FinishSpawningActor 사이에서 호출해야 한다. */
	void InitializeFromSpawner(
		UItemDefinitionDataAsset* InItemDefinition,
		int32 InQuantity,
		FName InSpawnerStateKey);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

private:
	/** 기본 Pickup 상호작용이 끝난 뒤 변경된 수량을 Subsystem에 반영한다. */
	UFUNCTION()
	void HandleSpawnerInteraction(AActor* Interactor);

	FName SpawnerStateKey = NAME_None;
};
