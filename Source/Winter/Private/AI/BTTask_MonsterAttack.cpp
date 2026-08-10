#include "AI/BTTask_MonsterAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Monster/BaseMonster.h"

UBTTask_MonsterAttack::UBTTask_MonsterAttack()
{
	NodeName = TEXT("Monster Attack");

	// [빌드 오류 수정] protected BlackboardKey를 현재 자식 클래스 타입을 통해 검사한다.
	BlackboardKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_MonsterAttack, BlackboardKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_MonsterAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABaseMonster* Monster = AIController ? Cast<ABaseMonster>(AIController->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName))
		: nullptr;

	if (!Monster || Monster->IsDead() || !IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	// [Behavior Tree 변경] 실제 쿨다운과 GAS 적용은 BaseMonster가 보장하며 Tree는 공격 시도 후 계속 평가한다.
	Monster->TryAttack(TargetActor);
	return EBTNodeResult::Succeeded;
}
