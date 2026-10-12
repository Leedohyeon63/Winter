#include "Actor/ItemSpawner.h"

#include "Actor/PickupActor.h"
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
	TSubclassOf<APickupActor> SelectedPickupClass;
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
		if (!SpawnTable->Entries.IsValidIndex(SelectedEntryIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner] Saved entry is missing. Key=%s Entry=%d"),
				*SpawnerStateKey.ToString(), SelectedEntryIndex);
			return false;
		}
		SelectedPickupClass = ResolvePickupClass(SpawnTable->Entries[SelectedEntryIndex]);
		SelectedQuantity = ExistingState->RemainingQuantity;
		SelectedRandomYaw = ExistingState->RandomYaw;
	}
	else
	{
		const int32 RandomSeed = bUseFixedSeed
			? HashCombine(FixedSeed, GetTypeHash(SpawnerStateKey))
			: StateSubsystem->MakeRandomSeed(SpawnerStateKey);
		FRandomStream RandomStream(RandomSeed);

		if (!TrySelectWeightedEntry(RandomStream, SelectedEntryIndex, SelectedPickupClass))
		{
			return false;
		}
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
		SelectedPickupClass,
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

bool AItemSpawner::TrySelectWeightedEntry(
	FRandomStream& RandomStream,
	int32& OutEntryIndex,
	TSubclassOf<APickupActor>& OutPickupClass) const
{
	OutEntryIndex = INDEX_NONE;
	OutPickupClass = nullptr;
	if (!SpawnTable)
	{
		return false;
	}

	struct FCandidate
	{
		int32 EntryIndex;
		float Weight;
		TSubclassOf<APickupActor> PickupClass;
	};
	TArray<FCandidate, TInlineAllocator<32>> Candidates;
	bool bHasInvalidEntry = false;
	const float EmptyWeight = FMath::Max(0.0f, SpawnTable->EmptyWeight);
	float TotalWeight = EmptyWeight;
	for (int32 Index = 0; Index < SpawnTable->Entries.Num(); ++Index)
	{
		const FWeightedItemSpawnEntry& Entry = SpawnTable->Entries[Index];
		// Zero-weight entries are intentionally disabled.
		if (Entry.Weight <= 0.0f)
		{
			continue;
		}

		const TSubclassOf<APickupActor> PickupClass = ResolvePickupClass(Entry);
		if (!FMath::IsFinite(Entry.Weight) || !PickupClass)
		{
			bHasInvalidEntry = true;
			UE_LOG(LogTemp, Warning,
				TEXT("[ItemSpawner] Invalid entry skipped. Spawner=%s Table=%s Entry=%d: check Weight, PickupActorClass and the BP default ItemDefinition."),
				*GetPathName(), *GetPathNameSafe(SpawnTable), Index);
			continue;
		}
		Candidates.Add({Index, Entry.Weight, PickupClass});
		TotalWeight += Entry.Weight;
	}

	// Misconfigured active entries must not turn into a persistent empty result.
	// An explicitly empty-only table (no active entries, positive EmptyWeight) is valid.
	if ((Candidates.IsEmpty() && bHasInvalidEntry)
		|| !FMath::IsFinite(SpawnTable->EmptyWeight)
		|| !FMath::IsFinite(TotalWeight) || TotalWeight <= 0.0f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ItemSpawner] Invalid spawn table; no result saved. Spawner=%s Table=%s: configure a valid pickup entry or EmptyWeight."),
			*GetPathName(), *GetPathNameSafe(SpawnTable));
		return false;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	if (Roll < EmptyWeight || Candidates.IsEmpty())
	{
		return true;
	}
	Roll -= EmptyWeight;
	for (const FCandidate& Candidate : Candidates)
	{
		if (Roll < Candidate.Weight)
		{
			OutEntryIndex = Candidate.EntryIndex;
			OutPickupClass = Candidate.PickupClass;
			return true;
		}
		Roll -= Candidate.Weight;
	}

	// Reuse the last validated candidate for floating-point boundary rounding.
	OutEntryIndex = Candidates.Last().EntryIndex;
	OutPickupClass = Candidates.Last().PickupClass;
	return true;
}


bool AItemSpawner::SpawnSelectedEntry(
	const TSubclassOf<APickupActor> PickupClass,
	const int32 Quantity,
	const float RandomYaw,
	const FName SpawnerStateKey)
{
	if (!GetWorld())
	{
		return false;
	}

	if (!PickupClass)
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

	// [아이템 스포너 추가] 아이템 데이터는 BP 기본값을 유지하고, BeginPlay 전에 수량과 스폰 기록만 설정한다.
	APickupActor* DeferredPickup = GetWorld()->SpawnActorDeferred<APickupActor>(
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

	DeferredPickup->InitializeFromSpawner(Quantity, SpawnerStateKey);
	SpawnedPickup = Cast<APickupActor>(UGameplayStatics::FinishSpawningActor(
		DeferredPickup,
		SpawnTransform));

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemSpawner] Spawned. Key=%s Item=%s Quantity=%d"),
		*SpawnerStateKey.ToString(),
		*GetNameSafe(DeferredPickup->GetItemDefinition()),
		Quantity);

	return IsValid(SpawnedPickup);
}

TSubclassOf<APickupActor> AItemSpawner::ResolvePickupClass(const FWeightedItemSpawnEntry& Entry) const
{
	const TSubclassOf<APickupActor> PickupClass = Entry.PickupActorClass
		? Entry.PickupActorClass : DefaultPickupActorClass;
	if (!PickupClass || PickupClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	// Item data belongs to the pickup BP. Exclude unconfigured BPs from the lottery.
	const APickupActor* Defaults = PickupClass->GetDefaultObject<APickupActor>();
	return Defaults && IsValid(Defaults->GetItemDefinition()) ? PickupClass : nullptr;
}
