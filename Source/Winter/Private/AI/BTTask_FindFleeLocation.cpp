#include "AI/BTTask_FindFleeLocation.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"

UBTTask_FindFleeLocation::UBTTask_FindFleeLocation()
{
	NodeName = TEXT("Find Flee Location");

	BlackboardKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FindFleeLocation, BlackboardKey));

	ThreatActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FindFleeLocation, ThreatActorKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FindFleeLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* ThreatActor = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(ThreatActorKey.SelectedKeyName))
		: nullptr;
	UNavigationSystemV1* NavigationSystem = ControlledPawn
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld())
		: nullptr;

	if (!ControlledPawn || !Blackboard || !IsValid(ThreatActor) || !NavigationSystem)
	{
		return EBTNodeResult::Failed;
	}

	bool bFoundCandidate = false;
	FVector BestLocation = ControlledPawn->GetActorLocation();
	float BestDistanceSquared = FVector::DistSquared(BestLocation, ThreatActor->GetActorLocation());

	for (int32 Index = 0; Index < FMath::Max(1, CandidateCount); ++Index)
	{
		FNavLocation CandidateLocation;
		if (!NavigationSystem->GetRandomReachablePointInRadius(
			ControlledPawn->GetActorLocation(),
			FleeSearchRadius,
			CandidateLocation))
		{
			continue;
		}

		const float CandidateDistanceSquared = FVector::DistSquared(
			CandidateLocation.Location,
			ThreatActor->GetActorLocation());

		if (!bFoundCandidate || CandidateDistanceSquared > BestDistanceSquared)
		{
			bFoundCandidate = true;
			BestDistanceSquared = CandidateDistanceSquared;
			BestLocation = CandidateLocation.Location;
		}
	}

	if (!bFoundCandidate)
	{
		return EBTNodeResult::Failed;
	}

	// [도주 추가] 무작위 후보 중 위협과 가장 먼 도달 가능 지점을 Move To에 전달한다.
	Blackboard->SetValueAsVector(BlackboardKey.SelectedKeyName, BestLocation);
	return EBTNodeResult::Succeeded;
}
