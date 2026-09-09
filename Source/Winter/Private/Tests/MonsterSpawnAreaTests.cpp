#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Actor/MonsterSpawnArea.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Monster/BaseMonster.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "PlayerCharacter.h"
#include "Subsystem/MonsterGenSubsystem.h"
#include "Subsystem/MonsterPoolSubsystem.h"
#include "UObject/UnrealType.h"

namespace MonsterSpawnAreaTests
{
	struct FFixture
	{
		FTestWorldWrapper Wrapper;
		UWorld* World = nullptr;
		bool Initialize()
		{
			if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
			World = Wrapper.GetTestWorld();
			World->CreateAISystem();
			World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
			return Wrapper.BeginPlayInTestWorld();
		}

		bool BuildNavigation()
		{
			AStaticMeshActor* Floor = World->SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FVector(0, 0, -50)));
			Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
			Floor->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(0, 0, -50), FVector(40, 40, 1)));
			ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
			UBoxComponent* BoundsBox = NewObject<UBoxComponent>(Bounds);
			Bounds->AddInstanceComponent(BoundsBox);
			BoundsBox->SetupAttachment(Bounds->GetRootComponent());
			BoundsBox->SetBoxExtent(FVector(2200, 2200, 1000));
			BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			BoundsBox->SetCanEverAffectNavigation(false);
			BoundsBox->RegisterComponent();
			FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Nav) return false;
			FindFProperty<FBoolProperty>(UNavigationSystemV1::StaticClass(), TEXT("bGenerateNavigationOnlyAroundNavigationInvokers"))->SetPropertyValue_InContainer(Nav, false);
			Nav->OnNavigationBoundsUpdated(Bounds);
			Nav->Build();
			ANavigationData* Data = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
			if (!Data) return false;
			Data->EnsureBuildCompletion();
			FNavLocation Projected;
			return Nav->ProjectPointToNavigation(FVector(500, 0, 0), Projected, FVector(100, 100, 300));
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterSpawnAreaGeometryTest, "Winter.MonsterSpawnArea.Geometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterSpawnAreaGeometryTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	AMonsterSpawnArea* Area = Fixture.World->SpawnActor<AMonsterSpawnArea>();
	Area->SpawnBounds->SetBoxExtent(FVector(200, 100, 100));
	Area->SetActorLocation(FVector(1100, 0, 0));
	TestTrue(TEXT("Area edge intersects even when center is outside player range"), Area->IntersectsPlayerRange(FVector::ZeroVector, 1000));
	TestTrue(TEXT("Point inside both bounds is allowed"), Area->IsSpawnLocationAllowed(FVector(950, 0, 0), FVector::ZeroVector, 1000));
	TestFalse(TEXT("Area point outside player range is rejected"), Area->IsSpawnLocationAllowed(FVector(1200, 0, 0), FVector::ZeroVector, 1000));
	TestFalse(TEXT("Player-range point outside area is rejected"), Area->IsSpawnLocationAllowed(FVector(500, 0, 0), FVector::ZeroVector, 1000));
	Area->SetActorTransform(FTransform(FRotator(0, 45, 0), FVector(1000, 1000, 0), FVector(2, 1, 1)));
	const FVector Inside = Area->GetActorTransform().TransformPosition(FVector(190, 90, 0));
	const FVector Outside = Area->GetActorTransform().TransformPosition(FVector(210, 0, 0));
	TestTrue(TEXT("Rotated scaled area contains its local interior"), Area->IsSpawnLocationAllowed(Inside, FVector::ZeroVector, 10000));
	TestFalse(TEXT("Rotated area rejects local exterior"), Area->IsSpawnLocationAllowed(Outside, FVector::ZeroVector, 10000));
	TestFalse(TEXT("Zero activation range disables spawning"), Area->IntersectsPlayerRange(Inside, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterSpawnAreaLifecycleTest, "Winter.MonsterSpawnArea.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterSpawnAreaLifecycleTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	UMonsterPoolSubsystem* Pool = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>();
	UMonsterGenSubsystem* Spawner = Fixture.World->GetSubsystem<UMonsterGenSubsystem>();
	AMonsterSpawnArea* Area = Fixture.World->SpawnActor<AMonsterSpawnArea>();
	Area->MonsterClass = ABaseMonster::StaticClass();
	TestEqual(TEXT("Expanded player spawn radius"), Spawner->GetActiveSpawnRadius(), 10000.0f);
	TestEqual(TEXT("Despawn hysteresis"), Spawner->GetActiveDespawnRadius(), 15000.0f);
	TestFalse(TEXT("Missing navmesh cannot spawn"), Area->TrySpawnMonster(FVector::ZeroVector, 10000, *Pool));
	ABaseMonster* Monster = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(500, 0, 100)));
	Area->SpawnedMonsters.Add({Monster, Monster->GetLifeGeneration()});
	TestEqual(TEXT("Area tracks owned monster"), Area->GetActiveMonsterCount(), 1);
	Area->RefreshMonsters(FVector::ZeroVector, 15000, *Pool);
	TestTrue(TEXT("Within retention range stays active"), Monster->IsActiveMonster());
	Area->RefreshMonsters(FVector(20000, 0, 0), 15000, *Pool);
	TestFalse(TEXT("Beyond retention range returns to pool"), Monster->IsActiveMonster());
	TestEqual(TEXT("Returned instance no longer counted"), Spawner->GetActiveMonsterCount(), 0);
	Monster = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(500, 0, 100)));
	Area->SpawnedMonsters.Add({Monster, Monster->GetLifeGeneration()});
	Pool->ReleaseMonster(Monster);
	ABaseMonster* Reused = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(500, 0, 100)));
	TestEqual(TEXT("Pool reuses same actor"), Reused, Monster);
	Area->ReleaseAllMonsters(*Pool);
	TestTrue(TEXT("Old area cannot release another lifetime"), Reused->IsActiveMonster());
	Area->SpawnedMonsters.Add({Reused, Reused->GetLifeGeneration()});
	Area->bSpawnEnabled = false;
	Area->RefreshMonsters(FVector::ZeroVector, 15000, *Pool);
	TestFalse(TEXT("Disabling area releases owned population"), Reused->IsActiveMonster());
	Area->Destroy();
	TestEqual(TEXT("Destroyed area unregisters"), Spawner->GetActiveMonsterCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterSpawnAreaNavigationTest, "Winter.MonsterSpawnArea.NavigationAndPopulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterSpawnAreaNavigationTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	if (!TestTrue(TEXT("Build actual navigation on test floor"), Fixture.BuildNavigation())) return false;
	APlayerCharacter* Player = Fixture.World->SpawnActor<APlayerCharacter>(FVector(-500, 0, 100), FRotator::ZeroRotator);
	Fixture.World->SpawnActor<APlayerController>()->Possess(Player);
	UMonsterGenSubsystem* Spawner = Fixture.World->GetSubsystem<UMonsterGenSubsystem>();
	Spawner->ManageMonsters();
	TestEqual(TEXT("No areas means no random player-centered spawns"), Spawner->GetActiveMonsterCount(), 0);
	AMonsterSpawnArea* Area = Fixture.World->SpawnActor<AMonsterSpawnArea>(FVector(500, 0, 300), FRotator::ZeroRotator);
	Area->SpawnBounds->SetBoxExtent(FVector(400, 400, 500));
	Area->MonsterClass = ABaseMonster::StaticClass();
	Area->MaxMonsters = 1;
	Spawner->ManageMonsters();
	TestEqual(TEXT("Nearby designated area spawns"), Area->GetActiveMonsterCount(), 1);
	for (TActorIterator<ABaseMonster> It(Fixture.World); It; ++It)
	{
		if (It->IsActiveMonster())
		{
			TestEqual(TEXT("Only configured monster class"), It->GetClass(), Area->MonsterClass.Get());
			TestTrue(TEXT("Spawn center inside area and player range"), Area->IsSpawnLocationAllowed(It->GetActorLocation(), Player->GetActorLocation(), Spawner->GetActiveSpawnRadius()));
			TestTrue(TEXT("Capsule placed above floor"), It->GetActorLocation().Z > 80);
		}
	}
	Spawner->ManageMonsters();
	TestEqual(TEXT("Per-area limit enforced"), Area->GetActiveMonsterCount(), 1);
	AMonsterSpawnArea* Second = Fixture.World->SpawnActor<AMonsterSpawnArea>(FVector(0, 1000, 300), FRotator::ZeroRotator);
	Second->SpawnBounds->SetBoxExtent(FVector(400, 400, 500));
	Second->MonsterClass = ABaseMonster::StaticClass();
	Spawner->ActiveMaxMonsters = 1;
	Spawner->ManageMonsters();
	TestEqual(TEXT("Shared population cap enforced"), Second->GetActiveMonsterCount(), 0);
	Spawner->ActiveMaxMonsters = 10;
	Spawner->ManageMonsters();
	TestEqual(TEXT("Next eligible area gets a spawn"), Second->GetActiveMonsterCount(), 1);
	Player->SetActorLocation(FVector(30000, 0, 100));
	Spawner->ManageMonsters();
	TestEqual(TEXT("Distant area population reclaimed"), Spawner->GetActiveMonsterCount(), 0);
	return true;
}

#endif
