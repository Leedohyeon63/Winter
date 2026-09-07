#include "AI/MonsterAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "WinterGameplayTags.h"

AMonsterAIController::AMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	DamagePerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("DamagePerception"));
	SetPerceptionComponent(*DamagePerception);
	DamageSenseConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageSenseConfig"));
	DamageSenseConfig->Implementation = UAISense_Damage::StaticClass();
	DamagePerception->ConfigureSense(*DamageSenseConfig);
	DamagePerception->OnTargetPerceptionUpdated.AddDynamic(
		this, &AMonsterAIController::HandleTargetPerceptionUpdated);
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ResetPerceptionState();

	const ABaseMonster* Monster = Cast<ABaseMonster>(InPawn);
	if (!Monster || !Monster->IsActiveMonster())
	{
		// [몬스터 풀링 추가] 예열 생성된 비활성 몬스터는 빙의만 유지하고 BT를 시작하지 않는다.
		DeactivatePooledMonster();
		return;
	}

	DamagePerception->SetSenseEnabled(UAISense_Damage::StaticClass(), true);
	RunAssignedBehaviorTree();
}

void AMonsterAIController::OnUnPossess()
{
	ResetPerceptionState();
	DamagePerception->SetSenseEnabled(UAISense_Damage::StaticClass(), false);
	if (UBrainComponent* ActiveBrainComponent = GetBrainComponent())
	{
		ActiveBrainComponent->StopLogic(TEXT("Monster unpossessed"));
	}

	StopMovement();
	Super::OnUnPossess();
}

void AMonsterAIController::ActivatePooledMonster()
{
	ResetPerceptionState();
	DamagePerception->SetSenseEnabled(UAISense_Damage::StaticClass(), true);
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	RunAssignedBehaviorTree();
}

void AMonsterAIController::DeactivatePooledMonster()
{
	DamagePerception->SetSenseEnabled(UAISense_Damage::StaticClass(), false);
	ResetPerceptionState();
	if (UBrainComponent* ActiveBrainComponent = GetBrainComponent())
	{
		ActiveBrainComponent->StopLogic(TEXT("Monster returned to pool"));
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

bool AMonsterAIController::RunAssignedBehaviorTree()
{
	const ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
	UBehaviorTree* BehaviorTreeAsset = Monster && Monster->IsActiveMonster()
		? Monster->GetBehaviorTreeAsset()
		: nullptr;

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] BehaviorTreeAsset is not assigned."));
		return false;
	}

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		if (BehaviorTreeAsset->BlackboardAsset)
		{
			// 같은 에셋의 InitializeBlackboard는 값을 유지하므로 키를 명시적으로 초기화한다.
			BlackboardComponent->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
			for (int32 KeyIndex = 0; KeyIndex < BehaviorTreeAsset->BlackboardAsset->GetNumKeys(); ++KeyIndex)
			{
				BlackboardComponent->ClearValue(static_cast<FBlackboard::FKey>(KeyIndex));
			}
			BlackboardComponent->SetValueAsObject(FBlackboard::KeySelf, GetPawn());
		}
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MonsterAI] Failed to run Behavior Tree."));
		return false;
	}
	return true;
}

void AMonsterAIController::ResetPerceptionState()
{
	DamageInstigator.Reset();
	DamageInstigatorGeneration = 0;
	DamagePerception->ForgetAll();
}

AActor* AMonsterAIController::GetDamageInstigator() const
{
	AActor* Actor = DamageInstigator.Get();
	if (!IsValid(Actor) || Actor == GetPawn())
	{
		return nullptr;
	}
	const ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (Monster && (!Monster->IsActiveMonster() || Monster->IsDead()
		|| Monster->GetLifeGeneration() != DamageInstigatorGeneration))
	{
		return nullptr;
	}
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Actor);
	const UAbilitySystemComponent* AbilitySystem = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
	return AbilitySystem && !AbilitySystem->HasMatchingGameplayTag(WinterGameplayTags::State_Dead) ? Actor : nullptr;
}

void AMonsterAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
	if (!Monster || !Monster->IsActiveMonster() || Monster->IsDead()
		|| !Stimulus.WasSuccessfullySensed() || Stimulus.Strength <= 0.0f
		|| Stimulus.Type != UAISense::GetSenseID<UAISense_Damage>()
		|| !IsValid(Actor) || Actor == Monster)
	{
		return;
	}
	// GAS가 보고한 지연 이벤트가 풀 재사용 후 도착하면 무시한다. 기본 외부 Damage 이벤트도 허용한다.
	if (Stimulus.Tag.GetComparisonIndex() == Monster->GetDamagePerceptionTag().GetComparisonIndex()
		&& Stimulus.Tag != Monster->GetDamagePerceptionTag())
	{
		return;
	}
	DamageInstigator = Actor;
	const ABaseMonster* AttackerMonster = Cast<ABaseMonster>(Actor);
	DamageInstigatorGeneration = AttackerMonster ? AttackerMonster->GetLifeGeneration() : 0;
	if (!GetDamageInstigator())
	{
		DamageInstigator.Reset();
		return;
	}
	Monster->CancelAttackForTargetChange(Actor);
	if (Monster->GetDisposition() == EMonsterDisposition::Passive)
	{
		Monster->StartFleeing();
	}
	else if (Monster->GetDisposition() == EMonsterDisposition::Neutral)
	{
		Monster->Provoke();
	}
}

AActor* AMonsterAIController::SelectCombatTarget()
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
	if (!Monster || !Monster->IsActiveMonster() || Monster->IsDead()
		|| Monster->GetDisposition() == EMonsterDisposition::Passive)
	{
		return nullptr;
	}
	const float RangeSquared = FMath::Square(Monster->GetAggroRange());
	AActor* Attacker = GetDamageInstigator();
	if (Attacker && FVector::DistSquared(Monster->GetActorLocation(), Attacker->GetActorLocation()) <= RangeSquared
		&& Monster->CanEngageTarget(Attacker))
	{
		return Attacker;
	}
	if (DamageInstigator.IsValid() || DamageInstigator.IsStale())
	{
		DamageInstigator.Reset();
		Monster->ResetProvocation();
	}

	if (Monster->GetDisposition() == EMonsterDisposition::Aggressive)
	{
		// 물리 공간 검색으로 주변 Pawn만 검사한다. 같은/높은 레벨과 비활성 개체는 제외한다.
		TArray<FOverlapResult> NearbyPawns;
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MonsterPreySearch), false, Monster);
		GetWorld()->OverlapMultiByObjectType(NearbyPawns, Monster->GetActorLocation(), FQuat::Identity,
			ObjectQuery, FCollisionShape::MakeSphere(FMath::Max(0.0f, Monster->GetAggroRange())), QueryParams);
		ABaseMonster* NearestPrey = nullptr;
		float NearestDistanceSquared = RangeSquared;
		for (const FOverlapResult& Result : NearbyPawns)
		{
			ABaseMonster* Candidate = Cast<ABaseMonster>(Result.GetActor());
			if (!Monster->CanHuntMonster(Candidate) || !Monster->CanEngageTarget(Candidate))
			{
				continue;
			}
			const float DistanceSquared = FVector::DistSquared(Monster->GetActorLocation(), Candidate->GetActorLocation());
			if (DistanceSquared <= NearestDistanceSquared)
			{
				NearestDistanceSquared = DistanceSquared;
				NearestPrey = Candidate;
			}
		}
		if (NearestPrey)
		{
			return NearestPrey;
		}
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	return Player && Monster->CanEngageTarget(Player)
		&& FVector::DistSquared(Monster->GetActorLocation(), Player->GetActorLocation()) <= RangeSquared ? Player : nullptr;
}
