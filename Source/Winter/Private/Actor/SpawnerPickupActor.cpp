#include "Actor/SpawnerPickupActor.h"

#include "Components/InteractableComponent.h"
#include "Engine/GameInstance.h"
#include "Subsystem/ItemSpawnStateSubsystem.h"

void ASpawnerPickupActor::InitializeFromSpawner(
	UItemDefinitionDataAsset* InItemDefinition,
	const int32 InQuantity,
	const FName InSpawnerStateKey)
{
	// [아이템 스포너 추가] 부모 Pickup의 BeginPlay 전에 데이터를 주입해 프롬프트가 즉시 정상화되게 한다.
	ItemDefinition = InItemDefinition;
	Quantity = FMath::Max(1, InQuantity);
	SpawnerStateKey = InSpawnerStateKey;
}

void ASpawnerPickupActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		// 부모의 HandleInteract가 먼저 실행되도록 Super::BeginPlay 이후에 등록한다.
		InteractableComponent->OnInteract.AddDynamic(
			this,
			&ASpawnerPickupActor::HandleSpawnerInteraction);
	}
}

void ASpawnerPickupActor::Destroyed()
{
	// [아이템 스포너 추가] 수량을 전부 주어서 파괴된 경우에만 스포너를 획득 완료로 기록한다.
	if (Quantity <= 0 && !SpawnerStateKey.IsNone())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UItemSpawnStateSubsystem* StateSubsystem =
				GameInstance->GetSubsystem<UItemSpawnStateSubsystem>())
			{
				StateSubsystem->MarkConsumed(SpawnerStateKey);
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[ItemSpawner] Pickup consumed. Key=%s"),
					*SpawnerStateKey.ToString());
			}
		}
	}

	Super::Destroyed();
}

void ASpawnerPickupActor::HandleSpawnerInteraction(AActor* Interactor)
{
	if (SpawnerStateKey.IsNone())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UItemSpawnStateSubsystem* StateSubsystem = GameInstance
		? GameInstance->GetSubsystem<UItemSpawnStateSubsystem>()
		: nullptr;

	if (!StateSubsystem)
	{
		return;
	}

	if (Quantity <= 0)
	{
		StateSubsystem->MarkConsumed(SpawnerStateKey);
	}
	else
	{
		// 인벤토리 공간이나 무게 제한 때문에 일부만 주운 경우를 보존한다.
		StateSubsystem->UpdateRemainingQuantity(SpawnerStateKey, Quantity);
	}
}
