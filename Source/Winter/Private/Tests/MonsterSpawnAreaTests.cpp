#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Actor/MonsterSpawnArea.h"
#include "AI/MonsterAIController.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "Attribute/PlayerStatAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/CapsuleComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "Combat/MonsterDamageEffect.h"
#include "Combat/WinterCombat.h"
#include "Perception/AIPerceptionSystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "GameState/MainGameState.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPassiveMonsterMovementTest, "Winter.MonsterEcology.PassiveMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPassiveMonsterMovementTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	if (!TestTrue(TEXT("Build navigation"), Fixture.BuildNavigation())) return false;
	UBehaviorTree* Tree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/AI/BT_Monster_Passive.BT_Monster_Passive"));
	if (!TestNotNull(TEXT("Saved passive tree loads"), Tree) || !TestNotNull(TEXT("Passive root"), Tree->RootNode.Get())) return false;
	if (!TestEqual(TEXT("Flee and wander branches exist"), Tree->RootNode->Children.Num(), 2)) return false;
	for (const FBTCompositeChild& Branch : Tree->RootNode->Children)
	{
		if (!TestNotNull(TEXT("Branch sequence"), Branch.ChildComposite.Get())) return false;
		if (!TestEqual(TEXT("Find destination, move, wait"), Branch.ChildComposite->Children.Num(), 3)) return false;
		TestTrue(TEXT("Each branch contains actual movement"), Branch.ChildComposite->Children[1].ChildTask->IsA<UBTTask_MoveTo>());
	}
	UMonsterPoolSubsystem* Pool = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>();
	ABaseMonster* Passive = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	if (!TestNotNull(TEXT("Passive monster"), Passive)) return false;
	FEnumProperty* Disposition = FindFProperty<FEnumProperty>(ABaseMonster::StaticClass(), TEXT("Disposition"));
	Disposition->GetUnderlyingProperty()->SetIntPropertyValue(Disposition->ContainerPtrToValuePtr<void>(Passive), static_cast<int64>(EMonsterDisposition::Passive));
	FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("PassiveBehaviorTreeAsset"))->SetObjectPropertyValue_InContainer(Passive, Tree);
	AMonsterAIController* Controller = Cast<AMonsterAIController>(Passive->GetController());
	if (!TestNotNull(TEXT("Monster controller"), Controller)) return false;
	Controller->ActivatePooledMonster();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	if (!TestNotNull(TEXT("Passive blackboard"), Blackboard)) return false;
	auto Advance = [&Fixture](float Seconds)
	{
		for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds * 60.0f); ++Frame)
		{
			Fixture.Wrapper.TickTestWorld(1.0f / 60.0f);
		}
	};
	const FVector Start = Passive->GetActorLocation();
	Advance(3.0f);
	TestTrue(TEXT("Unprovoked passive actually wanders on navmesh"), FVector::Dist2D(Start, Passive->GetActorLocation()) > 30.0f);
	TestNull(TEXT("Wandering passive has no combat target"), Blackboard->GetValueAsObject(TEXT("TargetActor")));
	ABaseMonster* Attacker = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(Passive->GetActorLocation() + FVector(250, 0, 0)));
	TestTrue(TEXT("Apply actual damage"), WinterCombat::ApplyDamageEffect(Attacker->GetAbilitySystemComponent(), Attacker,
		Passive, UMonsterDamageEffect::StaticClass(), 5.0f));
	UAIPerceptionSystem::GetCurrent(Fixture.World)->Tick(0.01f);
	const float InitialThreatDistance = FVector::Dist2D(Passive->GetActorLocation(), Attacker->GetActorLocation());
	Advance(0.6f);
	TestTrue(TEXT("Damage interrupts wandering and enters flee branch"), Blackboard->GetValueAsBool(TEXT("IsFleeing")));
	TestEqual(TEXT("Flee threat is actual attacker"), Blackboard->GetValueAsObject(TEXT("ThreatActor")), static_cast<UObject*>(Attacker));
	TestTrue(TEXT("Flee destination is set"), Blackboard->IsVectorValueSet(TEXT("FleeLocation")));
	TestNull(TEXT("Fleeing passive never targets attacker for combat"), Blackboard->GetValueAsObject(TEXT("TargetActor")));
	Advance(2.0f);
	TestTrue(TEXT("Passive physically moves away from attacker"),
		FVector::Dist2D(Passive->GetActorLocation(), Attacker->GetActorLocation()) > InitialThreatDistance + 50.0f);
	Advance(4.5f);
	TestFalse(TEXT("Flee timeout returns to wandering"), Blackboard->GetValueAsBool(TEXT("IsFleeing")));
	TestNull(TEXT("Expired flee threat is cleared"), Blackboard->GetValueAsObject(TEXT("ThreatActor")));
	Pool->ReleaseMonster(Passive);
	ABaseMonster* Reused = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	TestEqual(TEXT("Passive instance reused"), Reused, Passive);
	TestTrue(TEXT("Reused capsule restores query collision needed for walking"), Reused->GetCapsuleComponent()->IsQueryCollisionEnabled());
	Advance(2.0f);
	TestTrue(TEXT("Reused passive resumes wandering"), FVector::Dist2D(FVector::ZeroVector, Reused->GetActorLocation()) > 30.0f);
	TestFalse(TEXT("Reused passive has no old flee state"), Blackboard->GetValueAsBool(TEXT("IsFleeing")));
	Fixture.Wrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNeutralFoxCombatTest, "Winter.MonsterEcology.NeutralFoxCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNeutralFoxCombatTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize()) || !TestTrue(TEXT("Build navigation"), Fixture.BuildNavigation())) return false;
	UClass* FoxClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Fox.BP_Fox_C"));
	UClass* WeaponEffect = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/GAS/GE/Weapon/GE_WeaponDamge.GE_WeaponDamge_C"));
	if (!TestNotNull(TEXT("Fox blueprint"), FoxClass) || !TestNotNull(TEXT("Actual weapon effect"), WeaponEffect)) return false;
	ABaseMonster* Fox = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>()->AcquireMonster(FoxClass, FTransform(FVector(0, 0, 100)));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerCharacter* Player = Fixture.World->SpawnActor<APlayerCharacter>(FVector(600, 0, 100), FRotator::ZeroRotator, Spawn);
	Fixture.World->SpawnActor<APlayerController>()->Possess(Player);
	AMonsterAIController* Controller = Cast<AMonsterAIController>(Fox->GetController());
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	if (!TestNotNull(TEXT("Neutral tree blackboard"), Blackboard)) return false;
	auto Advance = [&Fixture](float Seconds)
	{
		for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds * 60); ++Frame) Fixture.Wrapper.TickTestWorld(1.0f / 60.0f);
	};
	Advance(0.5f);
	TestNull(TEXT("Unprovoked fox does not attack player"), Blackboard->GetValueAsObject(TEXT("TargetActor")));
	const float Before = Fox->GetAbilitySystemComponent()->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute());
	TestTrue(TEXT("Player weapon damages fox"), WinterCombat::ApplyDamageEffect(Player->GetAbilitySystemComponent(), Player, Fox, WeaponEffect, 5.0f));
	TestEqual(TEXT("Actual weapon reduces fox health"), Fox->GetAbilitySystemComponent()->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute()), Before - 5.0f);
	Advance(0.4f);
	TestTrue(TEXT("Damage perception provokes fox"), Fox->IsProvoked());
	TestEqual(TEXT("Neutral tree promptly targets attacker"), Blackboard->GetValueAsObject(TEXT("TargetActor")), static_cast<UObject*>(Player));
	const float PlayerHealth = Player->GetAbilitySystemComponent()->GetNumericAttribute(UPlayerStatAttributeSet::GetHealthAttribute());
	Advance(6.0f);
	TestTrue(TEXT("Fox chases and damages player through saved Neutral tree"), Player->GetAbilitySystemComponent()->GetNumericAttribute(UPlayerStatAttributeSet::GetHealthAttribute()) < PlayerHealth);
	Fixture.Wrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAggressiveWolfChaseTest, "Winter.MonsterEcology.AggressiveWolfChase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAggressiveWolfChaseTest::RunTest(const FString& Parameters)
{
	UClass* WolfClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Wolf.BP_Wolf_C"));
	UClass* DeerClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Deer2.BP_Deer2_C"));
	if (!TestNotNull(TEXT("Wolf blueprint"), WolfClass) || !TestNotNull(TEXT("Deer blueprint"), DeerClass)) return false;
	for (bool bPrey : {false, true})
	{
		MonsterSpawnAreaTests::FFixture Fixture;
		if (!TestTrue(TEXT("Create world"), Fixture.Initialize()) || !TestTrue(TEXT("Build navigation"), Fixture.BuildNavigation())) return false;
		ABaseMonster* Wolf = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>()->AcquireMonster(WolfClass, FTransform(FVector(0, 0, 100)));
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Target = nullptr;
		UAbilitySystemComponent* TargetASC = nullptr;
		FGameplayAttribute Health;
		if (bPrey)
		{
			ABaseMonster* Deer = Fixture.World->SpawnActor<ABaseMonster>(DeerClass, FVector(600, 0, 100), FRotator::ZeroRotator, Spawn);
			// Keep prey stationary to isolate the predator's chase completion and attack transition.
			Cast<AMonsterAIController>(Deer->GetController())->GetBrainComponent()->StopLogic(TEXT("Stationary chase target"));
			Target = Deer;
			TargetASC = Deer->GetAbilitySystemComponent();
			Health = UMonsterStatAttributeSet::GetHealthAttribute();
		}
		else
		{
			APlayerCharacter* Player = Fixture.World->SpawnActor<APlayerCharacter>(FVector(600, 0, 100), FRotator::ZeroRotator, Spawn);
			Fixture.World->SpawnActor<APlayerController>()->Possess(Player);
			Target = Player;
			TargetASC = Player->GetAbilitySystemComponent();
			Health = UPlayerStatAttributeSet::GetHealthAttribute();
		}
		const float Before = TargetASC->GetNumericAttribute(Health);
		bool bDamaged = false;
		for (int32 Frame = 0; Frame < 360; ++Frame)
		{
			Fixture.Wrapper.TickTestWorld(1.0f / 60.0f);
			if (TargetASC->GetNumericAttribute(Health) < Before) { bDamaged = true; break; }
		}
		AddInfo(FString::Printf(TEXT("%s: distance=%.1f attackRange=%.1f"), bPrey ? TEXT("Deer") : TEXT("Player"),
			FVector::Dist(Wolf->GetActorLocation(), Target->GetActorLocation()), Wolf->GetAttackRange()));
		TestTrue(bPrey ? TEXT("Wolf chases and damages deer") : TEXT("Wolf chases and damages player"), bDamaged);
		Fixture.Wrapper.ForwardErrorMessages(this);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMentalityWorldEffectsTest, "Winter.MonsterEcology.MentalityWorldEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMentalityWorldEffectsTest::RunTest(const FString& Parameters)
{
	MonsterSpawnAreaTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize()) || !TestTrue(TEXT("Build navigation"), Fixture.BuildNavigation())) return false;
	AMainGameState* State = Fixture.World->SpawnActor<AMainGameState>();
	Fixture.World->SetGameState(State);
	// The generic fixture starts with GameStateBase; connect the real game's state for this integration test.
	Fixture.World->GetSubsystem<UMonsterGenSubsystem>()->OnWorldBeginPlay(*Fixture.World);
	APlayerCharacter* Player = Fixture.World->SpawnActor<APlayerCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	APlayerController* PC = Fixture.World->SpawnActor<APlayerController>();
	PC->Possess(Player);
	UClass* Replacement = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Deer2.BP_Deer2_C"));
	if (!TestNotNull(TEXT("Replacement class"), Replacement)) return false;
	AMonsterSpawnArea* Area = Fixture.World->SpawnActor<AMonsterSpawnArea>(FVector(600, 0, 200), FRotator::ZeroRotator);
	Area->SpawnBounds->SetBoxExtent(FVector(250, 250, 400));
	Area->MonsterClass = ABaseMonster::StaticClass();
	Area->MaxMonsters = 1;
	Area->SpawnInterval = 0.1f;
	Area->MentalityMonsterOverrides.Add(EMentalityWorldState::Critical, Replacement);
	auto Advance = [&Fixture](float Seconds)
	{
		for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds * 60); ++Frame) Fixture.Wrapper.TickTestWorld(1.0f / 60.0f);
	};
	auto CheckPopulation = [&](UClass* Expected)
	{
		TestEqual(TEXT("One monster in area"), Area->GetActiveMonsterCount(), 1);
		for (TActorIterator<ABaseMonster> It(Fixture.World); It; ++It)
			if (It->IsActiveMonster()) TestEqual(TEXT("Only state-appropriate class remains active"), It->GetClass(), Expected);
	};
	Advance(1.5f);
	CheckPopulation(ABaseMonster::StaticClass());
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->SetNumericAttributeBase(UPlayerStatAttributeSet::GetMentalityAttribute(), 10.0f);
	TestEqual(TEXT("GAS change enters critical state"), State->CurrentMentalityWorldState, EMentalityWorldState::Critical);
	Advance(3.0f);
	CheckPopulation(Replacement);
	UPostProcessComponent* PP = Player->FindComponentByClass<UPostProcessComponent>();
	if (!TestNotNull(TEXT("Mentality post process exists"), PP)) return false;
	TestTrue(TEXT("Low mentality smoothly darkens local view"), PP->bEnabled && PP->BlendWeight > 0.8f && PP->Settings.AutoExposureBias < 0);
	ASC->SetNumericAttributeBase(UPlayerStatAttributeSet::GetMentalityAttribute(), 60.0f);
	TestEqual(TEXT("Unconfigured uneasy state falls back to base monster"), Area->GetEffectiveMonsterClass().Get(), ABaseMonster::StaticClass());
	Advance(1.5f);
	CheckPopulation(ABaseMonster::StaticClass());
	ASC->SetNumericAttributeBase(UPlayerStatAttributeSet::GetMentalityAttribute(), 100.0f);
	Advance(4.0f);
	TestEqual(TEXT("Recovery removes screen effect"), PP->BlendWeight, 0.0f);
	ASC->SetNumericAttributeBase(UPlayerStatAttributeSet::GetMaxMentalityAttribute(), 1000.0f);
	TestEqual(TEXT("Maximum mentality changes also update world state"), State->CurrentMentalityWorldState, EMentalityWorldState::Critical);
	PC->UnPossess();
	Advance(0.1f);
	TestFalse(TEXT("Unpossessed pawn cannot affect view"), PP->bEnabled);
	Fixture.Wrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
