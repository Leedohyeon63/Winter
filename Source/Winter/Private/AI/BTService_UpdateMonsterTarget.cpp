#include "AI/BTService_UpdateMonsterTarget.h"

#include "AI/MonsterAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
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

	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	ABaseMonster* Monster = AIController ? Cast<ABaseMonster>(AIController->GetPawn()) : nullptr;
	AActor* ThreatActor = AIController ? AIController->GetDamageInstigator() : nullptr;
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
		if (Monster)
		{
			Monster->CancelAttackForTargetChange(nullptr);
		}
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

	// 도주는 플레이어에 고정하지 않고 실제 피해를 준 공격자를 사용한다.
	if (Monster
		&& !Monster->IsDead()
		&& IsValid(ThreatActor)
		&& Monster->ShouldContinueFleeingFrom(ThreatActor))
	{
		ClearCombatTarget();
		if (bHasThreatActorKey)
		{
			Blackboard->SetValueAsObject(ThreatActorKey.SelectedKeyName, ThreatActor);
		}
		if (bHasFleeingKey)
		{
			Blackboard->SetValueAsBool(IsFleeingKey.SelectedKeyName, true);
		}
		return;
	}

	ClearFleeTarget();
	if (Monster && !IsValid(ThreatActor))
	{
		Monster->ResetFleeing();
	}

	AActor* TargetActor = AIController ? AIController->SelectCombatTarget() : nullptr;
	if (!Monster
		|| Monster->IsDead()
		|| !IsValid(TargetActor)
		|| !Monster->CanEngageTarget(TargetActor))
	{
		// [몬스터 성향 추가] 비선공과 아직 피격되지 않은 중립 몬스터는 Target을 만들지 않는다.
		ClearCombatTarget();
		return;
	}

	const float DistanceSquared =
		FVector::DistSquared(Monster->GetActorLocation(), TargetActor->GetActorLocation());

	if (DistanceSquared > FMath::Square(Monster->GetAggroRange()))
	{
		// [Behavior Tree 변경] 감지 범위를 벗어나면 Target을 지워 추적 Branch가 자동 중단되게 한다.
		ClearCombatTarget();
		return;
	}

	if (bHasTargetActorKey)
	{
		Monster->CancelAttackForTargetChange(TargetActor);
		Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, TargetActor);
	}
	AIController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);

	const bool bInAttackRange =
		DistanceSquared <= FMath::Square(Monster->GetAttackRange())
		&& AIController->LineOfSightTo(TargetActor);

	// [Behavior Tree 변경] 거리와 시야가 모두 충족돼야 공격 Sequence로 전환한다.
	if (bHasAttackRangeKey)
	{
		Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bInAttackRange);
	}
}
