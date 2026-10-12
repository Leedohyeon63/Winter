#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Actor/ItemSpawner.h"
#include "Actor/PickupActor.h"
#include "Actor/SpawnerPickupActor.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "DataAsset/ItemSpawnTableDataAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystem/ItemSpawnStateSubsystem.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemSpawnerSelectionTest, "Winter.ItemSpawner.SelectionAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemSpawnerSelectionTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!TestTrue(TEXT("Create game world"), Wrapper.CreateTestWorld(EWorldType::Game)))
	{
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	UItemSpawnStateSubsystem* State = World->GetGameInstance()->GetSubsystem<UItemSpawnStateSubsystem>();
	if (!TestNotNull(TEXT("Spawn state subsystem"), State))
	{
		return false;
	}

	// Use native pickup classes to avoid depending on project content or changing assets.
	struct FScopedPickupDefinition
	{
		APickupActor* Defaults = GetMutableDefault<ASpawnerPickupActor>();
		FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(APickupActor::StaticClass(), TEXT("ItemDefinition"));
		UObject* Original = Property->GetObjectPropertyValue_InContainer(Defaults);
		~FScopedPickupDefinition() { Property->SetObjectPropertyValue_InContainer(Defaults, Original); }
	} Definition;
	UItemDefinitionDataAsset* Item = NewObject<UItemDefinitionDataAsset>();
	Definition.Property->SetObjectPropertyValue_InContainer(Definition.Defaults, Item);

	auto MakeSpawner = [World](UItemSpawnTableDataAsset* Table, FName Id)
	{
		AItemSpawner* Spawner = World->SpawnActor<AItemSpawner>();
		FindFProperty<FObjectPropertyBase>(AItemSpawner::StaticClass(), TEXT("SpawnTable"))
			->SetObjectPropertyValue_InContainer(Spawner, Table);
		FindFProperty<FNameProperty>(AItemSpawner::StaticClass(), TEXT("SpawnerId"))
			->SetPropertyValue_InContainer(Spawner, Id);
		return Spawner;
	};
	auto Key = [World](const TCHAR* Id)
	{
		return FName(*FString::Printf(TEXT("%s::%s"), *World->GetMapName(), Id));
	};

	AddExpectedError(TEXT("Invalid entry skipped"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("Invalid spawn table; no result saved"), EAutomationExpectedErrorFlags::Contains, 2);

	UItemSpawnTableDataAsset* Table = NewObject<UItemSpawnTableDataAsset>();
	AItemSpawner* Retry = MakeSpawner(Table, TEXT("Retry"));
	TestFalse(TEXT("No weighted outcomes cannot spawn"), Retry->SpawnItem());
	TestNull(TEXT("Zero-total configuration is not saved as empty"), State->FindSpawnerState(Key(TEXT("Retry"))));

	FWeightedItemSpawnEntry Invalid;
	Invalid.PickupActorClass = APickupActor::StaticClass(); // No ItemDefinition.
	Table->Entries.Add(Invalid);
	Table->EmptyWeight = 100.0f;
	TestFalse(TEXT("Invalid-only candidates cannot spawn"), Retry->SpawnItem());
	TestNull(TEXT("Invalid candidates with EmptyWeight still do not save an empty result"),
		State->FindSpawnerState(Key(TEXT("Retry"))));

	Table->Entries[0].PickupActorClass = ASpawnerPickupActor::StaticClass();
	Table->Entries[0].MinQuantity = Table->Entries[0].MaxQuantity = 3;
	Table->EmptyWeight = 0.0f;
	TestTrue(TEXT("Correcting the same table permits retry"), Retry->SpawnItem());
	const FItemSpawnerRuntimeState* Retried = State->FindSpawnerState(Key(TEXT("Retry")));
	if (TestNotNull(TEXT("Successful retry saved"), Retried))
	{
		TestEqual(TEXT("Correct entry saved"), Retried->SelectedEntryIndex, 0);
		TestEqual(TEXT("Configured quantity saved"), Retried->RemainingQuantity, 3);
	}
	TestFalse(TEXT("Repeated call does not duplicate pickup"), Retry->SpawnItem());

	UItemSpawnTableDataAsset* Mixed = NewObject<UItemSpawnTableDataAsset>();
	Mixed->Entries.Add(Invalid);
	Mixed->Entries.Add(Table->Entries[0]);
	Invalid.Weight = 0.0f;
	Mixed->Entries.Add(Invalid); // Disabled invalid entries do not warn.
	AItemSpawner* MixedSpawner = MakeSpawner(Mixed, TEXT("Mixed"));
	TestTrue(TEXT("Valid entry still spawns alongside an invalid entry"), MixedSpawner->SpawnItem());
	const FItemSpawnerRuntimeState* MixedState = State->FindSpawnerState(Key(TEXT("Mixed")));
	if (TestNotNull(TEXT("Mixed result saved"), MixedState))
	{
		TestEqual(TEXT("Original table index survives filtering"), MixedState->SelectedEntryIndex, 1);
	}

	UItemSpawnTableDataAsset* Empty = NewObject<UItemSpawnTableDataAsset>();
	Empty->EmptyWeight = 1.0f;
	AItemSpawner* EmptySpawner = MakeSpawner(Empty, TEXT("Empty"));
	TestFalse(TEXT("Intentional empty outcome creates no actor"), EmptySpawner->SpawnItem());
	const FItemSpawnerRuntimeState* EmptyState = State->FindSpawnerState(Key(TEXT("Empty")));
	if (TestNotNull(TEXT("Intentional empty outcome is saved"), EmptyState))
	{
		TestEqual(TEXT("Empty marker saved"), EmptyState->SelectedEntryIndex, INDEX_NONE);
	}
	Empty->Entries.Add(Table->Entries[0]);
	Empty->EmptyWeight = 0.0f;
	TestFalse(TEXT("Saved empty outcome does not reroll"), EmptySpawner->SpawnItem());
	return true;
}

#endif