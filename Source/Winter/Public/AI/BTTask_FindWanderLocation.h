#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindWanderLocation.generated.h"

/** 현재 위치 주변에서 내비게이션으로 이동 가능한 배회 지점을 찾는다. */
UCLASS()
class WINTER_API UBTTask_FindWanderLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindWanderLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	// [배회 추가] 현재 위치를 중심으로 후보 지점을 찾을 최대 반경이다.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float WanderRadius = 1000.0f;
};
