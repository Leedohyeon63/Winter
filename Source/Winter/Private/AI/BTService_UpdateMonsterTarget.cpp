// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_UpdateMonsterTarget.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/BaseMonster.h"

UBTService_UpdateMonsterTarget::UBTService_UpdateMonsterTarget()
{
    NodeName = TEXT("Update Monster Target");

    bNotifyTick = true;
    Interval = 0.25f;
    RandomDeviation = 0.05f;

    TargetActorKey.AddObjectFilter(
        this,
        GET_MEMBER_NAME_CHECKED(
            UBTService_UpdateMonsterTarget,
            TargetActorKey),
        AActor::StaticClass());

    InAttackRangeKey.AddBoolFilter(
        this,
        GET_MEMBER_NAME_CHECKED(
            UBTService_UpdateMonsterTarget,
            InAttackRangeKey));
}

void UBTService_UpdateMonsterTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(
        OwnerComp,
        NodeMemory,
        DeltaSeconds);

    AAIController* AIController =
        OwnerComp.GetAIOwner();

    ABaseMonster* Monster =
        AIController
        ? Cast<ABaseMonster>(AIController->GetPawn())
        : nullptr;

    ACharacter* Player =
        UGameplayStatics::GetPlayerCharacter(
            &OwnerComp,
            0);

    UBlackboardComponent* Blackboard =
        OwnerComp.GetBlackboardComponent();

    if (!Blackboard)
    {
        return;
    }

    if (!Monster
        || Monster->IsDead()
        || !IsValid(Player))
    {
        Blackboard->ClearValue(
            TargetActorKey.SelectedKeyName);

        Blackboard->SetValueAsBool(
            InAttackRangeKey.SelectedKeyName,
            false);

        return;
    }

    const float DistanceSquared =
        FVector::DistSquared(
            Monster->GetActorLocation(),
            Player->GetActorLocation());

    if (DistanceSquared
        > FMath::Square(Monster->GetAggroRange()))
    {
        Blackboard->ClearValue(
            TargetActorKey.SelectedKeyName);

        Blackboard->SetValueAsBool(
            InAttackRangeKey.SelectedKeyName,
            false);

        AIController->ClearFocus(
            EAIFocusPriority::Gameplay);

        return;
    }

    Blackboard->SetValueAsObject(
        TargetActorKey.SelectedKeyName,
        Player);

    AIController->SetFocus(
        Player,
        EAIFocusPriority::Gameplay);

    const bool bInAttackRange =
        DistanceSquared
        <= FMath::Square(Monster->GetAttackRange())
        && AIController->LineOfSightTo(Player);

    Blackboard->SetValueAsBool(
        InAttackRangeKey.SelectedKeyName,
        bInAttackRange);
}
