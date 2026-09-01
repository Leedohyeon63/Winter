#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MonsterAIController.generated.h"

UCLASS()
class WINTER_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterAIController();

	// [몬스터 풀링 추가] 풀에서 나온 몬스터의 Blackboard를 초기화하고 Behavior Tree를 다시 시작한다.
	void ActivatePooledMonster();

	// [몬스터 풀링 추가] Controller는 유지한 채 이동, 포커스, Behavior Tree만 정지한다.
	void DeactivatePooledMonster();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	FTimerHandle DecisionTimerHandle;

private:
	bool RunAssignedBehaviorTree();
};
