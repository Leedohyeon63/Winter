#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "AI/MonsterAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/MonsterDamageEffect.h"
#include "Combat/WinterCombat.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterDeathMontageTest, "Winter.MonsterEcology.DeathMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterDeathMontageTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.Initialize())) return false;
	UMonsterPoolSubsystem* Pool = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>();
	ABaseMonster* Monster = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	ABaseMonster* Attacker = Fixture.SpawnMonster(5, EMonsterDisposition::Aggressive, FVector(300, 0, 100));
	if (!TestNotNull(TEXT("Pooled monster"), Monster)) return false;

	// 실제 프로젝트 메시/몽타주의 임시 복사본으로 재생을 검증한다. 콘텐츠 에셋은 수정하지 않는다.
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/FirstPerson/FirstPersonArms/Character/Mesh/SK_Mannequin_Arms.SK_Mannequin_Arms"));
	UAnimMontage* SourceMontage = LoadObject<UAnimMontage>(nullptr,
		TEXT("/Game/FirstPerson/FirstPersonArms/Animations/FP_Rifle_Shoot_Montage.FP_Rifle_Shoot_Montage"));
	if (!TestNotNull(TEXT("Test mesh"), Mesh) || !TestNotNull(TEXT("Test montage"), SourceMontage)) return false;
	Monster->GetMesh()->SetSkeletalMesh(Mesh);
	Monster->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
	UAnimInstance* AnimInstance = Monster->GetMesh()->GetAnimInstance();
	if (!TestNotNull(TEXT("Animation instance"), AnimInstance)) return false;
	UAnimMontage* DeathMontage = DuplicateObject<UAnimMontage>(SourceMontage, GetTransientPackage());
	DeathMontage->RateScale = 0.5f;
	DeathMontage->bEnableAutoBlendOut = true;
	UAnimMontage* AttackMontage = DuplicateObject<UAnimMontage>(SourceMontage, GetTransientPackage());
	FObjectProperty* DeathProperty = FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("DeathMontage"));
	FFloatProperty* DelayProperty = FindFProperty<FFloatProperty>(ABaseMonster::StaticClass(), TEXT("DestroyDelayAfterDeath"));
	DeathProperty->SetObjectPropertyValue_InContainer(Monster, DeathMontage);
	DelayProperty->SetPropertyValue_InContainer(Monster, 0.0f);
	TestTrue(TEXT("Attack montage starts before death"), AnimInstance->Montage_Play(AttackMontage) > 0.0f);
	FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("PendingAttackTarget"))
		->SetObjectPropertyValue_InContainer(Monster, Attacker);

	FWorldFixture::Damage(Attacker, Monster, 1000.0f);
	TestTrue(TEXT("Lethal damage marks monster dead"), Monster->IsDead());
	TestTrue(TEXT("Death montage starts automatically"), AnimInstance->Montage_IsPlaying(DeathMontage));
	TestFalse(TEXT("Death stops attack montage"), AnimInstance->Montage_IsPlaying(AttackMontage));
	TestFalse(TEXT("Attack notify cannot damage after death"), Monster->ExecutePendingAttack());
	TestTrue(TEXT("Zero configured delay still retains death animation"), Monster->IsActiveMonster());
	TestTrue(TEXT("Return delay accounts for montage rate scale"), FMath::IsNearlyEqual(
		Monster->GetLifeSpan(), DeathMontage->GetPlayLength() / DeathMontage->RateScale));

	Pool->ReleaseMonster(Monster);
	TestFalse(TEXT("Early pool return stops death montage"), AnimInstance->Montage_IsActive(DeathMontage));
	ABaseMonster* Reused = Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	TestEqual(TEXT("Same monster reused"), Reused, Monster);
	TestFalse(TEXT("Reused monster alive"), Reused->IsDead());
	TestEqual(TEXT("Old death expiry cleared"), Reused->GetLifeSpan(), 0.0f);
	TestFalse(TEXT("Mesh animations resume on reuse"), Reused->GetMesh()->bPauseAnims);
	TestFalse(TEXT("Death montage does not resume"), AnimInstance->Montage_IsActive(DeathMontage));

	const float LongerDelay = DeathMontage->GetPlayLength() / DeathMontage->RateScale + 5.0f;
	DelayProperty->SetPropertyValue_InContainer(Monster, LongerDelay);
	FWorldFixture::Damage(Attacker, Monster, 1000.0f);
	TestTrue(TEXT("Longer configured corpse delay is preserved"), FMath::IsNearlyEqual(Monster->GetLifeSpan(), LongerDelay));
	// 에셋의 자동 블렌드 아웃이 켜져 있어도 실제 재생 종료 후 사망 포즈 가중치가 남아야 한다.
	for (int32 Frame = 0; Frame < FMath::CeilToInt((LongerDelay - 1.0f) * 60.0f); ++Frame)
	{
		AnimInstance->UpdateAnimation(1.0f / 60.0f, false);
	}
	TestTrue(TEXT("Shared montage asset keeps its original auto blend setting"), DeathMontage->bEnableAutoBlendOut);
	FAnimMontageInstance* DeathInstance = AnimInstance->GetActiveInstanceForMontage(DeathMontage);
	if (TestNotNull(TEXT("Death pose remains active after animation ends"), DeathInstance))
	{
		TestFalse(TEXT("Death animation reaches its end"), DeathInstance->IsPlaying());
		TestTrue(TEXT("Death pose retains full weight instead of blending to idle"), DeathInstance->GetWeight() > 0.99f);
	}
	Pool->ReleaseMonster(Monster);
	TestTrue(TEXT("Corpse is hidden on release"), Monster->IsHidden());
	Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	Monster->GetMesh()->SetAnimInstanceClass(nullptr);
	DelayProperty->SetPropertyValue_InContainer(Monster, 2.0f);
	FWorldFixture::Damage(Attacker, Monster, 1000.0f);
	TestEqual(TEXT("Unplayable montage falls back to configured delay"), Monster->GetLifeSpan(), 2.0f);
	Pool->ReleaseMonster(Monster);
	Pool->AcquireMonster(ABaseMonster::StaticClass(), FTransform(FVector(0, 0, 100)));
	DeathProperty->SetObjectPropertyValue_InContainer(Monster, nullptr);
	DelayProperty->SetPropertyValue_InContainer(Monster, 0.0f);
	FWorldFixture::Damage(Attacker, Monster, 1000.0f);
	TestFalse(TEXT("No montage and zero delay returns immediately"), Monster->IsActiveMonster());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeerDeathPoseTest, "Winter.MonsterEcology.DeerDeathPose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeerDeathPoseTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create isolated game world"), Fixture.Initialize())) return false;
	UClass* DeerClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Deer.BP_Deer_C"));
	UAnimSequence* DeathSequence = LoadObject<UAnimSequence>(nullptr,
		TEXT("/Game/Fab/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_Death.ANIM_DeerStag_Death"));
	if (!TestNotNull(TEXT("Actual deer blueprint"), DeerClass) || !TestNotNull(TEXT("Stag death sequence"), DeathSequence)) return false;
	UMonsterPoolSubsystem* Pool = Fixture.World->GetSubsystem<UMonsterPoolSubsystem>();
	ABaseMonster* Deer = Pool->AcquireMonster(DeerClass, FTransform(FVector(0, 0, 100)));
	if (!TestNotNull(TEXT("Pooled deer"), Deer)) return false;
	USkeletalMeshComponent* Mesh = Deer->GetMesh();
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!TestNotNull(TEXT("Deer animation blueprint instance"), AnimInstance)) return false;
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(DeathSequence, TEXT("DefaultSlot"));
	if (!TestNotNull(TEXT("Death montage using real stag animation"), Montage)) return false;
	Montage->bEnableAutoBlendOut = true;
	FindFProperty<FObjectProperty>(ABaseMonster::StaticClass(), TEXT("DeathMontage"))->SetObjectPropertyValue_InContainer(Deer, Montage);
	FindFProperty<FFloatProperty>(ABaseMonster::StaticClass(), TEXT("DestroyDelayAfterDeath"))
		->SetPropertyValue_InContainer(Deer, Montage->GetPlayLength() + 5.0f);
	// 애니메이션만 진행해 회수 타이머 이전/이후의 포즈를 별도로 검사한다.
	auto AdvancePose = [Mesh](float Seconds)
	{
		for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds * 60.0f); ++Frame)
		{
			Mesh->TickAnimation(1.0f / 60.0f, false);
			Mesh->RefreshBoneTransforms();
		}
	};
	AdvancePose(0.1f);
	const TArray<FTransform> StandingPose = Mesh->GetComponentSpaceTransforms();
	ABaseMonster* Attacker = Fixture.SpawnMonster(5, EMonsterDisposition::Aggressive, FVector(300, 0, 100));
	FWorldFixture::Damage(Attacker, Deer, 10000.0f);
	AdvancePose(Montage->GetPlayLength() + 0.5f);
	const TArray<FTransform> DeathPose = Mesh->GetComponentSpaceTransforms();
	TestTrue(TEXT("Deer death montage remains active after its end"), AnimInstance->Montage_IsActive(Montage));
	bool bPoseChanged = false;
	for (int32 Index = 0; Index < FMath::Min(StandingPose.Num(), DeathPose.Num()); ++Index)
	{
		bPoseChanged |= !StandingPose[Index].Equals(DeathPose[Index], 0.01f);
	}
	TestTrue(TEXT("Actual Anim Graph outputs a death pose different from standing"), bPoseChanged);
	AdvancePose(1.0f);
	bool bPoseHeld = DeathPose.Num() > 0 && DeathPose.Num() == Mesh->GetComponentSpaceTransforms().Num();
	for (int32 Index = 0; bPoseHeld && Index < DeathPose.Num(); ++Index)
	{
		bPoseHeld &= DeathPose[Index].Equals(Mesh->GetComponentSpaceTransforms()[Index], 0.01f);
	}
	TestTrue(TEXT("Actual deer bones keep the death pose during corpse delay"), bPoseHeld);
	Pool->ReleaseMonster(Deer);
	TestTrue(TEXT("Dead deer hidden before reuse"), Deer->IsHidden());
	TestFalse(TEXT("Death montage cleared while hidden"), AnimInstance->Montage_IsActive(Montage));
	ABaseMonster* ReusedDeer = Pool->AcquireMonster(DeerClass, FTransform(FVector(0, 0, 100)));
	TestEqual(TEXT("Same deer object reused"), ReusedDeer, Deer);
	TestFalse(TEXT("Reused deer is alive"), ReusedDeer->IsDead());
	TestFalse(TEXT("Reused deer animation is unpaused"), Mesh->bPauseAnims);
	AdvancePose(0.5f);
	TestFalse(TEXT("Reused deer no longer has a death montage"), AnimInstance->Montage_IsActive(Montage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWolfDeerAttackTest, "Winter.MonsterEcology.WolfDeerAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWolfDeerAttackTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	UClass* WolfClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Wolf.BP_Wolf_C"));
	UClass* DeerClass = LoadClass<ABaseMonster>(nullptr, TEXT("/Game/BluePrint/Mob/BP_Deer2.BP_Deer2_C"));
	if (!TestNotNull(TEXT("Wolf blueprint"), WolfClass) || !TestNotNull(TEXT("Deer blueprint"), DeerClass)) return false;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABaseMonster* Wolf = Fixture.World->SpawnActor<ABaseMonster>(WolfClass, FVector(0, 0, 100), FRotator::ZeroRotator, Spawn);
	ABaseMonster* Deer = Fixture.World->SpawnActor<ABaseMonster>(DeerClass, FVector(140, 0, 100), FRotator::ZeroRotator, Spawn);
	// Balance settings in the assets may change; keep this repeated-hit scenario deterministic.
	FindFProperty<FFloatProperty>(ABaseMonster::StaticClass(), TEXT("AttackDamage"))->SetPropertyValue_InContainer(Wolf, 10.0f);
	Deer->GetAbilitySystemComponent()->SetNumericAttributeBase(UMonsterStatAttributeSet::GetMaxHealthAttribute(), 100.0f);
	Deer->GetAbilitySystemComponent()->SetNumericAttributeBase(UMonsterStatAttributeSet::GetHealthAttribute(), 100.0f);
	for (ABaseMonster* Monster : {Wolf, Deer})
	{
		Monster->GetCharacterMovement()->DisableMovement();
		if (AAIController* AI = Cast<AAIController>(Monster->GetController()))
		{
			if (AI->GetBrainComponent()) AI->GetBrainComponent()->StopLogic(TEXT("Controlled melee test"));
		}
		AddInfo(FString::Printf(TEXT("%s capsule query=%d channel=%d active=%d"), *Monster->GetName(),
			Monster->GetCapsuleComponent()->IsQueryCollisionEnabled(), Monster->GetCapsuleComponent()->GetCollisionObjectType(), Monster->IsActiveMonster()));
	}
	for (int32 Attack = 0; Attack < 10; ++Attack)
	{
		TestTrue(FString::Printf(TEXT("Attack %d lands"), Attack + 1), Wolf->TryAttack(Deer));
		TestEqual(FString::Printf(TEXT("Health after attack %d"), Attack + 1),
			Deer->GetAbilitySystemComponent()->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute()), 100.0f - (Attack + 1) * 10.0f);
		if (Attack < 9) for (int32 Frame = 0; Frame < 90; ++Frame) Fixture.Wrapper.TickTestWorld(1.0f / 60.0f);
	}
	TestTrue(TEXT("Deer dies from repeated wolf attacks"), Deer->IsDead());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterInitialHealthTest, "Winter.MonsterEcology.InitialHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterInitialHealthTest::RunTest(const FString& Parameters)
{
	using namespace MonsterEcologyTests;
	FWorldFixture Fixture;
	if (!TestTrue(TEXT("Create world"), Fixture.Initialize())) return false;
	ABaseMonster* Attacker = Fixture.SpawnMonster(5, EMonsterDisposition::Aggressive, FVector(1000, 0, 100));
	for (float ConfiguredHealth : {250.0f, 60.0f, 0.0f})
	{
		const FTransform Transform(FVector(0, ConfiguredHealth * 10, 100));
		ABaseMonster* Monster = Fixture.World->SpawnActorDeferred<ABaseMonster>(ABaseMonster::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		FindFProperty<FFloatProperty>(ABaseMonster::StaticClass(), TEXT("InitialHealth"))->SetPropertyValue_InContainer(Monster, ConfiguredHealth);
		Monster->FinishSpawning(Transform);
		const float ExpectedHealth = FMath::Max(1.0f, ConfiguredHealth);
		UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
		TestEqual(TEXT("Spawn health follows individual setting"), ASC->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute()), ExpectedHealth);
		TestEqual(TEXT("Maximum health follows individual setting"), ASC->GetNumericAttribute(UMonsterStatAttributeSet::GetMaxHealthAttribute()), ExpectedHealth);
		FWorldFixture::Damage(Attacker, Monster, 0.5f);
		TestEqual(TEXT("Damage reduces configured health"), ASC->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute()), ExpectedHealth - 0.5f);
		Monster->AssignToPool(Fixture.World->GetSubsystem<UMonsterPoolSubsystem>());
		TestTrue(TEXT("Reactivate monster"), Monster->ActivateFromPool(Transform));
		TestEqual(TEXT("Reuse restores configured health"), ASC->GetNumericAttribute(UMonsterStatAttributeSet::GetHealthAttribute()), ExpectedHealth);
	}
	return true;
}

#endif
