// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_MonsterAttack.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Monster/BaseMonster.h"

UBTTask_MonsterAttack::UBTTask_MonsterAttack()
{
    NodeName = TEXT("Monster Attack");
    BlackboardKey.AddObjectFilter(
        this,
        GET_MEMBER_NAME_CHECKED(
            UBTTask_MonsterAttack,
            BlackboardKey),
        AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_MonsterAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController =
        OwnerComp.GetAIOwner();

    ABaseMonster* Monster =
        AIController
        ? Cast<ABaseMonster>(AIController->GetPawn())
        : nullptr;

    UBlackboardComponent* Blackboard =
        OwnerComp.GetBlackboardComponent();

    AActor* TargetActor =
        Blackboard
        ? Cast<AActor>(
            Blackboard->GetValueAsObject(
                BlackboardKey.SelectedKeyName))
        : nullptr;

    if (!Monster
        || Monster->IsDead()
        || !IsValid(TargetActor))
    {
        return EBTNodeResult::Failed;
    }

    Monster->TryAttack(TargetActor);
    return EBTNodeResult::Succeeded;
}
