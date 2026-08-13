#include "AI/BTService_UpdateMonsterTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"

UBTService_UpdateMonsterTarget::UBTService_UpdateMonsterTarget()
{
	NodeName = TEXT("Update Monster Target");

	// [Behavior Tree 변경] 플레이어 탐색은 매 프레임이 아니라 BT Service 주기로 실행한다.
	bNotifyTick = true;
	Interval = 0.25f;
	RandomDeviation = 0.05f;

	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateMonsterTarget, TargetActorKey),
		AActor::StaticClass());

	InAttackRangeKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateMonsterTarget, InAttackRangeKey));

	ThreatActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateMonsterTarget, ThreatActorKey),
		AActor::StaticClass());

	IsFleeingKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateMonsterTarget, IsFleeingKey));
}

void UBTService_UpdateMonsterTarget::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	ABaseMonster* Monster = AIController ? Cast<ABaseMonster>(AIController->GetPawn()) : nullptr;
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(&OwnerComp, 0);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!Blackboard)
	{
		return;
	}

	// [하위 호환] 새 도주 키를 지정하지 않은 기존 BT 에셋에서는 이름이 None이므로 해당 값만 건드리지 않는다.
	const bool bHasTargetActorKey = TargetActorKey.SelectedKeyName != NAME_None;
	const bool bHasAttackRangeKey = InAttackRangeKey.SelectedKeyName != NAME_None;
	const bool bHasThreatActorKey = ThreatActorKey.SelectedKeyName != NAME_None;
	const bool bHasFleeingKey = IsFleeingKey.SelectedKeyName != NAME_None;

	auto ClearCombatTarget = [&]()
	{
		if (bHasTargetActorKey)
		{
			Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
		}
		if (bHasAttackRangeKey)
		{
			Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		}
		if (AIController)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	};

	auto ClearFleeTarget = [&]()
	{
		if (bHasThreatActorKey)
		{
			Blackboard->ClearValue(ThreatActorKey.SelectedKeyName);
		}
		if (bHasFleeingKey)
		{
			Blackboard->SetValueAsBool(IsFleeingKey.SelectedKeyName, false);
		}
	};

	// [비선공 도주 추가] 도주 중에는 전투 Target을 만들지 않고 플레이어를 Threat로만 제공한다.
	if (Monster
		&& !Monster->IsDead()
		&& IsValid(Player)
		&& Monster->ShouldContinueFleeingFrom(Player))
	{
		ClearCombatTarget();
		if (bHasThreatActorKey)
		{
			Blackboard->SetValueAsObject(ThreatActorKey.SelectedKeyName, Player);
		}
		if (bHasFleeingKey)
		{
			Blackboard->SetValueAsBool(IsFleeingKey.SelectedKeyName, true);
		}
		return;
	}

	ClearFleeTarget();

	if (!Monster
		|| Monster->IsDead()
		|| !IsValid(Player)
		|| !Monster->CanEngageTarget(Player))
	{
		// [몬스터 성향 추가] 비선공과 아직 피격되지 않은 중립 몬스터는 Target을 만들지 않는다.
		ClearCombatTarget();
		return;
	}

	const float DistanceSquared =
		FVector::DistSquared(Monster->GetActorLocation(), Player->GetActorLocation());

	if (DistanceSquared > FMath::Square(Monster->GetAggroRange()))
	{
		// [Behavior Tree 변경] 감지 범위를 벗어나면 Target을 지워 추적 Branch가 자동 중단되게 한다.
		ClearCombatTarget();
		return;
	}

	if (bHasTargetActorKey)
	{
		Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, Player);
	}
	AIController->SetFocus(Player, EAIFocusPriority::Gameplay);

	const bool bInAttackRange =
		DistanceSquared <= FMath::Square(Monster->GetAttackRange())
		&& AIController->LineOfSightTo(Player);

	// [Behavior Tree 변경] 거리와 시야가 모두 충족돼야 공격 Sequence로 전환한다.
	if (bHasAttackRangeKey)
	{
		Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bInAttackRange);
	}
}
