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

	if (!Monster
		|| Monster->IsDead()
		|| !IsValid(Player)
		|| !Monster->CanEngageTarget(Player))
	{
		// [몬스터 성향 추가] 비선공과 아직 피격되지 않은 중립 몬스터는 Target을 만들지 않는다.
		Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
		Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		if (AIController)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
		return;
	}

	const float DistanceSquared =
		FVector::DistSquared(Monster->GetActorLocation(), Player->GetActorLocation());

	if (DistanceSquared > FMath::Square(Monster->GetAggroRange()))
	{
		// [Behavior Tree 변경] 감지 범위를 벗어나면 Target을 지워 추적 Branch가 자동 중단되게 한다.
		Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
		Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, Player);
	AIController->SetFocus(Player, EAIFocusPriority::Gameplay);

	const bool bInAttackRange =
		DistanceSquared <= FMath::Square(Monster->GetAttackRange())
		&& AIController->LineOfSightTo(Player);

	// [Behavior Tree 변경] 거리와 시야가 모두 충족돼야 공격 Sequence로 전환한다.
	Blackboard->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bInAttackRange);
}
