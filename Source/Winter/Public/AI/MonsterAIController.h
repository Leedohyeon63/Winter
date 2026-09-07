#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MonsterAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Damage;

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

	/** 피격 어그로를 먼저 선택하고, 선공 몬스터는 낮은 레벨의 먹잇감을 탐색한다. */
	AActor* SelectCombatTarget();
	AActor* GetDamageInstigator() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Perception")
	TObjectPtr<UAIPerceptionComponent> DamagePerception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageSenseConfig;

private:
	bool RunAssignedBehaviorTree();
	void ResetPerceptionState();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	TWeakObjectPtr<AActor> DamageInstigator;
	uint32 DamageInstigatorGeneration = 0;
};
