#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "AI/MonsterAIController.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/MonsterDamageEffect.h"
#include "Combat/WinterCombat.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Monster/BaseMonster.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Damage.h"
#include "PlayerCharacter.h"
#include "Subsystem/MonsterPoolSubsystem.h"
#include "UObject/UnrealType.h"

namespace MonsterEcologyTests
{
	struct FWorldFixture
	{
		FTestWorldWrapper Wrapper;
		UWorld* World = nullptr;

		bool Initialize()
		{
			if (!Wrapper.CreateTestWorld(EWorldType::Game))
			{
				return false;
			}
			World = Wrapper.GetTestWorld();
			World->CreateAISystem();
			World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
			return Wrapper.BeginPlayInTestWorld() && UAIPerceptionSystem::GetCurrent(World) != nullptr;
		}

		ABaseMonster* SpawnMonster(int32 Level, EMonsterDisposition Disposition, FVector Location)
		{
			ABaseMonster* Monster = World->SpawnActorDeferred<ABaseMonster>(ABaseMonster::StaticClass(),
				FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			SetDefaults(Monster, Level, Disposition);
			Monster->FinishSpawning(FTransform(Location));
			return Monster;
		}

		static void SetDefaults(ABaseMonster* Monster, int32 Level, EMonsterDisposition Disposition)
		{
			FindFProperty<FIntProperty>(ABaseMonster::StaticClass(), TEXT("MonsterLevel"))->SetPropertyValue_InContainer(Monster, Level);
			FEnumProperty* DispositionProperty = FindFProperty<FEnumProperty>(ABaseMonster::StaticClass(), TEXT("Disposition"));
			DispositionProperty->GetUnderlyingProperty()->SetIntPropertyValue(
				DispositionProperty->ContainerPtrToValuePtr<void>(Monster), static_cast<int64>(Disposition));
		}

		APlayerCharacter* SpawnPlayer(FVector Location)
		{
			FActorSpawnParameters Parameters;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			APlayerCharacter* Player = World->SpawnActor<APlayerCharacter>(Location, FRotator::ZeroRotator, Parameters);
			World->SpawnActor<APlayerController>()->Possess(Player);
			return Player;
		}

		void ProcessDamage()
		{
			UAIPerceptionSystem::GetCurrent(World)->Tick(0.01f);
		}

		static bool Damage(AActor* Source, ABaseMonster* Target, float Amount = 5.0f)
		{
			return WinterCombat::ApplyDamageEffect(Cast<IAbilitySystemInterface>(Source)->GetAbilitySystemComponent(),
				Source, Target, UMonsterDamageEffect::StaticClass(), Amount);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterEcologyAggroTest, "Winter.MonsterEcology.PredationAndDamageAggro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterEcologyAggroTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.Initialize())) return false;
	ABaseMonster* Wolf = Fixture.SpawnMonster(5, EMonsterDisposition::Aggressive, FVector(0, 0, 100));
	ABaseMonster* Deer = Fixture.SpawnMonster(1, EMonsterDisposition::Passive, FVector(140, 0, 100));
	ABaseMonster* Peer = Fixture.SpawnMonster(5, EMonsterDisposition::Aggressive, FVector(0, 300, 100));
	ABaseMonster* Stronger = Fixture.SpawnMonster(8, EMonsterDisposition::Aggressive, FVector(0, -300, 100));
	APlayerCharacter* Player = Fixture.SpawnPlayer(FVector(-100, 0, 100));
	AMonsterAIController* Controller = Cast<AMonsterAIController>(Wolf->GetController());
	if (!TestNotNull(TEXT("Monster AI controller"), Controller)) return false;
	TestFalse(TEXT("Cannot hunt self"), Wolf->CanHuntMonster(Wolf));
	TestFalse(TEXT("Cannot hunt equal level"), Wolf->CanEngageTarget(Peer));
	TestFalse(TEXT("Cannot hunt higher level"), Wolf->CanEngageTarget(Stronger));
	TestEqual(TEXT("Prey wins over nearer player"), Controller->SelectCombatTarget(), static_cast<AActor*>(Deer));
	TestTrue(TEXT("Actual monster melee attack"), Wolf->TryAttack(Deer));
	TestEqual(TEXT("Native effect damages monster health"), Deer->GetAbilitySystemComponent()->GetNumericAttribute(
		UMonsterStatAttributeSet::GetHealthAttribute()), 90.0f);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Deer perceives real wolf attacker"), Cast<AMonsterAIController>(Deer->GetController())->GetDamageInstigator(), static_cast<AActor*>(Wolf));
	TestTrue(TEXT("Passive flees after perceived damage"), Deer->IsFleeing());
	TestFalse(TEXT("Passive never retaliates"), Deer->CanEngageTarget(Wolf));

	FWorldFixture::Damage(Player, Wolf);
	TestNull(TEXT("GAS damage waits for Perception processing"), Controller->GetDamageInstigator());
	Fixture.ProcessDamage();
	TestEqual(TEXT("Player damage overrides prey"), Controller->SelectCombatTarget(), static_cast<AActor*>(Player));
	TestEqual(TEXT("Aggro remains on later evaluations"), Controller->SelectCombatTarget(), static_cast<AActor*>(Player));
	// Notify를 기다리는 공격의 대상만 주입해 피격 처리에 의한 취소 여부를 검증한다.
	FObjectProperty* PendingTarget = FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("PendingAttackTarget"));
	PendingTarget->SetObjectPropertyValue_InContainer(Wolf, Player);
	FWorldFixture::Damage(Player, Wolf);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Repeated damage by same target preserves pending attack"), PendingTarget->GetObjectPropertyValue_InContainer(Wolf), static_cast<UObject*>(Player));
	FWorldFixture::Damage(Peer, Wolf);
	Fixture.ProcessDamage();
	TestNull(TEXT("New attacker cancels previous target's pending attack"), PendingTarget->GetObjectPropertyValue_InContainer(Wolf));
	FWorldFixture::Damage(Player, Wolf);
	Fixture.ProcessDamage();
	Player->SetActorLocation(FVector(10000, 0, 100));
	TestEqual(TEXT("Out-of-range attacker releases aggro"), Controller->SelectCombatTarget(), static_cast<AActor*>(Deer));
	FWorldFixture::Damage(Stronger, Wolf);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Retaliation bypasses prey level restriction"), Controller->SelectCombatTarget(), static_cast<AActor*>(Stronger));
	FWorldFixture::Damage(Peer, Wolf);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Latest attacker takes aggro"), Controller->SelectCombatTarget(), static_cast<AActor*>(Peer));
	FWorldFixture::Damage(Player, Peer, 1000.0f);
	TestEqual(TEXT("Dead attacker releases aggro"), Controller->SelectCombatTarget(), static_cast<AActor*>(Deer));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterEcologyNeutralTest, "Winter.MonsterEcology.NeutralAndPassive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterEcologyNeutralTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.Initialize())) return false;
	ABaseMonster* Neutral = Fixture.SpawnMonster(5, EMonsterDisposition::Neutral, FVector(0, 0, 100));
	ABaseMonster* Attacker = Fixture.SpawnMonster(1, EMonsterDisposition::Aggressive, FVector(300, 0, 100));
	APlayerCharacter* Player = Fixture.SpawnPlayer(FVector(-100, 0, 100));
	AMonsterAIController* Controller = Cast<AMonsterAIController>(Neutral->GetController());
	TestNull(TEXT("Neutral does not hunt lower levels or player"), Controller->SelectCombatTarget());
	FWorldFixture::Damage(Attacker, Neutral);
	Fixture.ProcessDamage();
	TestTrue(TEXT("Perception provokes neutral"), Neutral->IsProvoked());
	TestEqual(TEXT("Neutral retaliates against real attacker"), Controller->SelectCombatTarget(), static_cast<AActor*>(Attacker));
	TestFalse(TEXT("Neutral does not blame uninvolved player"), Neutral->CanEngageTarget(Player));
	Attacker->SetActorLocation(FVector(10000, 0, 100));
	TestNull(TEXT("Neutral calms after losing attacker"), Controller->SelectCombatTarget());
	TestFalse(TEXT("Provocation clears with lost damage aggro"), Neutral->IsProvoked());
	Neutral->Provoke();
	TestEqual(TEXT("Explicit legacy Provoke still targets player"), Controller->SelectCombatTarget(), static_cast<AActor*>(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterEcologyPoolTest, "Winter.MonsterEcology.PoolResetsAggro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterEcologyPoolTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.Initialize())) return false;
	UMonsterPoolSubsystem* Pool = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>();
	ABaseMonster* Wolf = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	ABaseMonster* Attacker = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(300, 0, 100)));
	FWorldFixture::SetDefaults(Wolf, 5, EMonsterDisposition::Aggressive);
	FWorldFixture::SetDefaults(Attacker, 8, EMonsterDisposition::Aggressive);
	AMonsterAIController* Controller = Cast<AMonsterAIController>(Wolf->GetController());
	FWorldFixture::Damage(Attacker, Wolf);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Initial damage aggro"), Controller->GetDamageInstigator(), static_cast<AActor*>(Attacker));
	Pool->ReleaseMonster(Attacker);
	ABaseMonster* ReusedAttacker = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(300, 0, 100)));
	TestEqual(TEXT("Attacker object reused"), ReusedAttacker, Attacker);
	TestNull(TEXT("New attacker lifetime has no inherited hostility"), Controller->GetDamageInstigator());

	// 지연된 Damage Sense 이벤트가 피해자의 다음 생애에 도착하는 경우도 검사한다.
	FWorldFixture::Damage(Attacker, Wolf);
	Pool->ReleaseMonster(Wolf);
	ABaseMonster* ReusedWolf = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	TestEqual(TEXT("Victim object reused"), ReusedWolf, Wolf);
	Fixture.ProcessDamage();
	TestNull(TEXT("Queued previous-life damage is ignored"), Controller->GetDamageInstigator());
	FWorldFixture::Damage(Attacker, Wolf);
	Fixture.ProcessDamage();
	TestEqual(TEXT("Damage sense works after reuse"), Controller->GetDamageInstigator(), static_cast<AActor*>(Attacker));

	UBehaviorTree* Tree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/AI/BT_Monster_Aggressive.BT_Monster_Aggressive"));
	if (TestNotNull(TEXT("Existing aggressive tree loads"), Tree))
	{
		FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("AggressiveBehaviorTreeAsset"))->SetObjectPropertyValue_InContainer(Wolf, Tree);
		Controller->ActivatePooledMonster();
		UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
		if (TestNotNull(TEXT("Existing tree blackboard"), Blackboard))
		{
			Blackboard->SetValueAsObject(TEXT("TargetActor"), Attacker);
			Blackboard->SetValueAsBool(TEXT("IsInAttackRange"), true);
			Pool->ReleaseMonster(Wolf);
			Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
			TestNull(TEXT("Reused blackboard target cleared"), Blackboard->GetValueAsObject(TEXT("TargetActor")));
			TestFalse(TEXT("Reused blackboard range cleared"), Blackboard->GetValueAsBool(TEXT("IsInAttackRange")));
			TestEqual(TEXT("SelfActor preserved"), Blackboard->GetValueAsObject(TEXT("SelfActor")), static_cast<UObject*>(Wolf));
		}
	}
	return true;
}

#endif
