// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateMonsterTarget.generated.h"

/** 플레이어 전투 타깃과 비선공 몬스터의 도주 위협을 주기적으로 갱신한다. */
UCLASS()
class WINTER_API UBTService_UpdateMonsterTarget : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateMonsterTarget();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector InAttackRangeKey;

	// [비선공 도주 추가] 비선공 몬스터가 피격된 뒤 도망칠 대상을 저장한다. 기존 BT에서는 None으로 두어도 된다.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ThreatActorKey;

	// [비선공 도주 추가] Passive BT의 도주 Branch를 제어한다. 기존 BT에서는 None으로 두어도 된다.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector IsFleeingKey;
};
