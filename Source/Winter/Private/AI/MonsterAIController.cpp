#include "AI/MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "Monster/BaseMonster.h"


AMonsterAIController::AMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const ABaseMonster* Monster =
		Cast<ABaseMonster>(InPawn);

	UBehaviorTree* BehaviorTreeAsset =
		Monster ? Monster->GetBehaviorTreeAsset() : nullptr;

	if (!BehaviorTreeAsset)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[MonsterAI] BehaviorTreeAsset is not assigned."));

		return;
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[MonsterAI] Failed to run Behavior Tree."));
	}
}

void AMonsterAIController::OnUnPossess()
{
	if (UBrainComponent* Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Monster unpossessed"));
	}

	StopMovement();
	Super::OnUnPossess();
}

