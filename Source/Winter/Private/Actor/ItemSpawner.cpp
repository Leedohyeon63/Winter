#include "Actor/ItemSpawner.h"

#include "Actor/SpawnerPickupActor.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "DataAsset/ItemSpawnTableDataAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/ItemSpawnStateSubsystem.h"

AItemSpawner::AItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetupAttachment(SceneRoot);

	DefaultPickupActorClass = ASpawnerPickupActor::StaticClass();
}

void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnItem();
}

bool AItemSpawner::SpawnItem()
{
	if (IsValid(SpawnedPickup))
	{
		return false;
	}

	if (!SpawnTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner] SpawnTable is empty. Spawner=%s"), *GetName());
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UItemSpawnStateSubsystem* StateSubsystem = GameInstance
		? GameInstance->GetSubsystem<UItemSpawnStateSubsystem>()
		: nullptr;

	if (!StateSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemSpawner] ItemSpawnStateSubsystem is unavailable."));
		return false;
	}

	const FName SpawnerStateKey = BuildSpawnerStateKey();
	int32 SelectedEntryIndex = INDEX_NONE;
	int32 SelectedQuantity = 0;
	float SelectedRandomYaw = 0.0f;

	if (const FItemSpawnerRuntimeState* ExistingState =
		StateSubsystem->FindSpawnerState(SpawnerStateKey))
	{
		// [아이템 스포너 추가] 이미 주운 스포너 또는 최초 추첨에서 빈 결과가 나온 스포너는 그대로 비워 둔다.
		if (ExistingState->bConsumed || ExistingState->SelectedEntryIndex == INDEX_NONE)
		{
			return false;
		}

		SelectedEntryIndex = ExistingState->SelectedEntryIndex;
		SelectedQuantity = ExistingState->RemainingQuantity;
		SelectedRandomYaw = ExistingState->RandomYaw;
	}
	else
	{
		const int32 RandomSeed = bUseFixedSeed
			? HashCombine(FixedSeed, GetTypeHash(SpawnerStateKey))
			: StateSubsystem->MakeRandomSeed(SpawnerStateKey);
		FRandomStream RandomStream(RandomSeed);

		SelectedEntryIndex = SelectWeightedEntry(RandomStream);
		if (!SpawnTable->Entries.IsValidIndex(SelectedEntryIndex))
		{
			// INDEX_NONE을 저장해 레벨을 다시 열어도 재추첨하지 않게 한다.
			StateSubsystem->SaveSelection(SpawnerStateKey, INDEX_NONE, 0, 0.0f);
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[ItemSpawner] Empty result. Key=%s"),
				*SpawnerStateKey.ToString());
			return false;
		}

		const FWeightedItemSpawnEntry& Entry = SpawnTable->Entries[SelectedEntryIndex];
		const int32 MinQuantity = FMath::Max(1, FMath::Min(Entry.MinQuantity, Entry.MaxQuantity));
		const int32 MaxQuantity = FMath::Max(MinQuantity, FMath::Max(Entry.MinQuantity, Entry.MaxQuantity));
		SelectedQuantity = RandomStream.RandRange(MinQuantity, MaxQuantity);
		SelectedRandomYaw = bRandomYaw ? RandomStream.FRandRange(0.0f, 360.0f) : 0.0f;

		StateSubsystem->SaveSelection(
			SpawnerStateKey,
			SelectedEntryIndex,
			SelectedQuantity,
			SelectedRandomYaw);
	}

	if (SelectedQuantity <= 0)
	{
		StateSubsystem->MarkConsumed(SpawnerStateKey);
		return false;
	}

	return SpawnSelectedEntry(
		SelectedEntryIndex,
		SelectedQuantity,
		SelectedRandomYaw,
		SpawnerStateKey);
}

FName AItemSpawner::BuildSpawnerStateKey() const
{
	FString MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("UnknownMap");
	if (GetWorld())
	{
		// PIE의 UEDPIE_0_ 접두사를 제거해 에디터 실행 방식과 관계없이 같은 키를 사용한다.
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	}

	const FName EffectiveSpawnerId = SpawnerId.IsNone() ? GetFName() : SpawnerId;
	return FName(*FString::Printf(TEXT("%s::%s"), *MapName, *EffectiveSpawnerId.ToString()));
}

int32 AItemSpawner::SelectWeightedEntry(FRandomStream& RandomStream) const
{
	if (!SpawnTable)
	{
		return INDEX_NONE;
	}

	float TotalWeight = FMath::Max(0.0f, SpawnTable->EmptyWeight);
	for (const FWeightedItemSpawnEntry& Entry : SpawnTable->Entries)
	{
		if (Entry.ItemDefinition && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return INDEX_NONE;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	const float EmptyWeight = FMath::Max(0.0f, SpawnTable->EmptyWeight);
	if (Roll < EmptyWeight)
	{
		return INDEX_NONE;
	}
	Roll -= EmptyWeight;

	for (int32 EntryIndex = 0; EntryIndex < SpawnTable->Entries.Num(); ++EntryIndex)
	{
		const FWeightedItemSpawnEntry& Entry = SpawnTable->Entries[EntryIndex];
		if (!Entry.ItemDefinition || Entry.Weight <= 0.0f)
		{
			continue;
		}

		if (Roll < Entry.Weight)
		{
			return EntryIndex;
		}
		Roll -= Entry.Weight;
	}

	// 부동소수점 경계값 때문에 끝까지 도달한 경우 마지막 유효 항목을 선택한다.
	for (int32 EntryIndex = SpawnTable->Entries.Num() - 1; EntryIndex >= 0; --EntryIndex)
	{
		const FWeightedItemSpawnEntry& Entry = SpawnTable->Entries[EntryIndex];
		if (Entry.ItemDefinition && Entry.Weight > 0.0f)
		{
			return EntryIndex;
		}
	}

	return INDEX_NONE;
}

bool AItemSpawner::SpawnSelectedEntry(
	const int32 EntryIndex,
	const int32 Quantity,
	const float RandomYaw,
	const FName SpawnerStateKey)
{
	if (!SpawnTable || !SpawnTable->Entries.IsValidIndex(EntryIndex) || !GetWorld())
	{
		return false;
	}

	const FWeightedItemSpawnEntry& Entry = SpawnTable->Entries[EntryIndex];
	TSubclassOf<ASpawnerPickupActor> PickupClass = Entry.PickupActorClass;
	if (!PickupClass)
	{
		PickupClass = DefaultPickupActorClass;
	}

	if (!PickupClass || !Entry.ItemDefinition)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemSpawner] Pickup class or ItemDefinition is invalid. Key=%s"),
			*SpawnerStateKey.ToString());
		return false;
	}

	const FVector SpawnLocation = GetActorTransform().TransformPosition(SpawnOffset);
	FRotator FinalRotation = GetActorRotation() + SpawnRotation;
	FinalRotation.Yaw += RandomYaw;
	const FTransform SpawnTransform(FinalRotation, SpawnLocation);

	// [아이템 스포너 추가] BeginPlay 전에 ItemDefinition과 Quantity를 넣기 위해 Deferred Spawn을 사용한다.
	ASpawnerPickupActor* DeferredPickup = GetWorld()->SpawnActorDeferred<ASpawnerPickupActor>(
		PickupClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!DeferredPickup)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemSpawner] Failed to spawn pickup. Key=%s"), *SpawnerStateKey.ToString());
		return false;
	}

	DeferredPickup->InitializeFromSpawner(Entry.ItemDefinition, Quantity, SpawnerStateKey);
	SpawnedPickup = Cast<ASpawnerPickupActor>(UGameplayStatics::FinishSpawningActor(
		DeferredPickup,
		SpawnTransform));

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemSpawner] Spawned. Key=%s Item=%s Quantity=%d"),
		*SpawnerStateKey.ToString(),
		*GetNameSafe(Entry.ItemDefinition),
		Quantity);

	return IsValid(SpawnedPickup);
}
