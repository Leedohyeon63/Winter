#include "AI/BTTask_FindWanderLocation.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"

UBTTask_FindWanderLocation::UBTTask_FindWanderLocation()
{
	NodeName = TEXT("Find Wander Location");

	BlackboardKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FindWanderLocation, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindWanderLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UNavigationSystemV1* NavigationSystem = ControlledPawn
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld())
		: nullptr;

	if (!ControlledPawn || !Blackboard || !NavigationSystem)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation WanderLocation;
	if (!NavigationSystem->GetRandomReachablePointInRadius(
		ControlledPawn->GetActorLocation(),
		WanderRadius,
		WanderLocation))
	{
		return EBTNodeResult::Failed;
	}

	// [배회 추가] Move To가 사용할 수 있도록 도달 가능한 월드 위치를 Blackboard에 기록한다.
	Blackboard->SetValueAsVector(BlackboardKey.SelectedKeyName, WanderLocation.Location);
	return EBTNodeResult::Succeeded;
}
