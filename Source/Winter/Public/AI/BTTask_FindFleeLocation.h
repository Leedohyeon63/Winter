#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindFleeLocation.generated.h"

/** 여러 내비게이션 후보 중 위협으로부터 가장 멀어지는 도주 지점을 선택한다. */
UCLASS()
class WINTER_API UBTTask_FindFleeLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindFleeLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ThreatActorKey;

	// [도주 추가] 현재 위치 주변에서 도주 후보를 찾을 반경이다.
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0"))
	float FleeSearchRadius = 1500.0f;

	// [도주 추가] 후보 수가 많을수록 더 좋은 방향을 찾지만 한 번의 Task 비용이 증가한다.
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "1", ClampMax = "32"))
	int32 CandidateCount = 12;
};
