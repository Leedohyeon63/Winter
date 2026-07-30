// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateMonsterTarget.generated.h"

/**
 * 
 */
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
};
