#include "AI/MonsterAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Monster/BaseMonster.h"

AMonsterAIController::AMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const ABaseMonster* Monster = Cast<ABaseMonster>(InPawn);
	if (!Monster || !Monster->IsActiveMonster())
	{
		// [몬스터 풀링 추가] 예열 생성된 비활성 몬스터는 빙의만 유지하고 BT를 시작하지 않는다.
		DeactivatePooledMonster();
		return;
	}

	RunAssignedBehaviorTree();
}

void AMonsterAIController::OnUnPossess()
{
	if (UBrainComponent* ActiveBrainComponent = GetBrainComponent())
	{
		ActiveBrainComponent->StopLogic(TEXT("Monster unpossessed"));
	}

	StopMovement();
	Super::OnUnPossess();
}

void AMonsterAIController::ActivatePooledMonster()
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	RunAssignedBehaviorTree();
}

void AMonsterAIController::DeactivatePooledMonster()
{
	if (UBrainComponent* ActiveBrainComponent = GetBrainComponent())
	{
		ActiveBrainComponent->StopLogic(TEXT("Monster returned to pool"));
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

bool AMonsterAIController::RunAssignedBehaviorTree()
{
	const ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
	UBehaviorTree* BehaviorTreeAsset = Monster && Monster->IsActiveMonster()
		? Monster->GetBehaviorTreeAsset()
		: nullptr;

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] BehaviorTreeAsset is not assigned."));
		return false;
	}

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		if (BehaviorTreeAsset->BlackboardAsset)
		{
			// [몬스터 풀링 추가] 이전 생애의 Target, 공격 범위, 도주 위치를 모두 기본값으로 되돌린다.
			BlackboardComponent->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		}
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] Failed to run Behavior Tree."));
		return false;
	}
	return true;
}
