#include "Monster/BaseMonster.h"

#include "AI/MonsterAIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "Combat/WinterCombat.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"
#include "WinterGameplayTags.h"

ABaseMonster::ABaseMonster()
{
	// [Behavior Tree 변경] 판단은 Behavior Tree Service/Task가 담당하므로 몬스터 액터 Tick은 사용하지 않는다.
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(false);

	AttributeSet = CreateDefaultSubobject<UMonsterStatAttributeSet>(TEXT("AttributeSet"));

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	bUseControllerRotationYaw = false;

	// [몬스터 추가] 동적 스폰 직후에도 AIController가 자동으로 빙의한다.
	AIControllerClass = AMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.6f));

	// [몬스터 추가] 외형이 없는 현재 단계에서 엔진 기본 Cube를 임시 몬스터 몸체로 사용한다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UBehaviorTree* ABaseMonster::GetBehaviorTreeAsset() const
{
	UBehaviorTree* SelectedTree = nullptr;

	// [몬스터 성향 추가] AIController가 하나의 Getter만 호출해도 현재 성향에 맞는 Tree를 선택한다.
	switch (Disposition)
	{
	case EMonsterDisposition::Aggressive:
		SelectedTree = AggressiveBehaviorTreeAsset.Get();
		break;

	case EMonsterDisposition::Passive:
		SelectedTree = PassiveBehaviorTreeAsset.Get();
		break;

	case EMonsterDisposition::Neutral:
		SelectedTree = NeutralBehaviorTreeAsset.Get();
		break;

	default:
		break;
	}

	// [호환 유지] 기존 BP_BaseMonster의 BehaviorTreeAsset 설정을 그대로 사용할 수 있게 한다.
	return SelectedTree ? SelectedTree : BehaviorTreeAsset.Get();
}

bool ABaseMonster::CanEngageTarget(AActor* TargetActor) const
{
	if (bIsDead || !IsValid(TargetActor))
	{
		return false;
	}

	IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetAbilitySystem = TargetAbilityInterface
		? TargetAbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!TargetAbilitySystem
		|| TargetAbilitySystem->HasMatchingGameplayTag(WinterGameplayTags::State_Dead))
	{
		// [사망 상태 추가] 플레이어가 사망하면 BT Service가 전투 Target을 자동으로 비우게 한다.
		return false;
	}

	switch (Disposition)
	{
	case EMonsterDisposition::Aggressive:
		return true;

	case EMonsterDisposition::Passive:
		return false;

	case EMonsterDisposition::Neutral:
		return bIsProvoked;

	default:
		return false;
	}
}

void ABaseMonster::Provoke()
{
	if (bIsDead
		|| Disposition != EMonsterDisposition::Neutral
		|| bIsProvoked)
	{
		return;
	}

	// [몬스터 성향 추가] 중립 상태는 첫 피격 후 계속 적대하며 필요하면 Blueprint에서 연출을 실행한다.
	bIsProvoked = true;
	OnProvoked();
}

void ABaseMonster::ResetProvocation()
{
	// [몬스터 성향 추가] 중립 몬스터만 다시 비적대 상태로 되돌릴 수 있다.
	if (Disposition == EMonsterDisposition::Neutral)
	{
		bIsProvoked = false;
	}
}

bool ABaseMonster::ShouldContinueFleeingFrom(AActor* ThreatActor)
{
	if (bIsDead
		|| Disposition != EMonsterDisposition::Passive
		|| !bIsFleeing
		|| !GetWorld())
	{
		return false;
	}

	const bool bDurationExpired = GetWorld()->GetTimeSeconds() >= FleeEndTime;
	const bool bReachedSafeDistance = IsValid(ThreatActor)
		&& FVector::DistSquared(GetActorLocation(), ThreatActor->GetActorLocation())
		>= FMath::Square(FleeSafeDistance);

	if (bDurationExpired || bReachedSafeDistance)
	{
		ResetFleeing();
		return false;
	}

	return true;
}

void ABaseMonster::StartFleeing()
{
	if (bIsDead
		|| Disposition != EMonsterDisposition::Passive
		|| !GetWorld())
	{
		return;
	}

	const bool bWasAlreadyFleeing = bIsFleeing;
	bIsFleeing = true;
	FleeEndTime = GetWorld()->GetTimeSeconds() + FleeDuration;

	// [비선공 도주 추가] 연속 피격은 도주 시간만 연장하고 시작 연출은 한 번만 실행한다.
	if (!bWasAlreadyFleeing)
	{
		OnFleeStarted();
	}
}

void ABaseMonster::ResetFleeing()
{
	bIsFleeing = false;
	FleeEndTime = 0.0f;
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (AbilitySystemComponent && AttributeSet)
	{
		// [몬스터 추가] GAS로 체력이 변경되면 UI 확장 지점과 사망 처리를 동시에 갱신한다.
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &ABaseMonster::HandleHealthChanged);

		OnMonsterHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
	}
}

bool ABaseMonster::TryAttack(AActor* TargetActor)
{
	if (bIsDead
		|| !IsValid(TargetActor)
		|| !CanEngageTarget(TargetActor)
		|| !AbilitySystemComponent
		|| !AttackDamageEffect
		|| AttackDamage <= 0.0f
		|| PendingAttackTarget
		|| !GetWorld())
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextAttackAllowedTime)
	{
		return false;
	}

	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceSquared > FMath::Square(AttackRange))
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));

	// [몬스터 공격 판정 보완] 공격 시작 시 대상만 보관하고 실제 피해는 Notify 또는 즉시 판정 함수에서 처리한다.
	PendingAttackTarget = TargetActor;
	NextAttackAllowedTime = CurrentTime + AttackCooldown;

	const float MontageDuration = AttackMontage ? PlayAnimMontage(AttackMontage.Get()) : 0.0f;
	// [몬스터 공격 판정 보완] C++ 몽타주 시작 후 Blueprint 이벤트는 음향·파티클 같은 추가 연출에 사용한다.
	OnAttackStarted(TargetActor);
	if (bExecuteAttackOnAnimNotify && MontageDuration > 0.0f)
	{
		PendingAttackMontage = AttackMontage;
		if (GetMesh())
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				FOnMontageEnded MontageEndedDelegate;
				MontageEndedDelegate.BindUObject(this, &ABaseMonster::HandleAttackMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, PendingAttackMontage.Get());
			}
		}
		return true;
	}

	return ExecutePendingAttack();
}

bool ABaseMonster::ExecutePendingAttack()
{
	AActor* TargetActor = PendingAttackTarget.Get();
	PendingAttackTarget = nullptr;
	PendingAttackMontage = nullptr;

	if (bIsDead
		|| !IsValid(TargetActor)
		|| !CanEngageTarget(TargetActor)
		|| !AbilitySystemComponent
		|| !AttackDamageEffect
		|| !GetWorld())
	{
		return false;
	}

	const FVector Start = GetActorLocation()
		+ FVector::UpVector * GetSimpleCollisionHalfHeight() * 0.5f;
	const FVector End = Start + GetActorForwardVector() * FMath::Max(0.0f, AttackRange);
	const float Radius = FMath::Max(1.0f, AttackRadius);

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MonsterMeleeSweep), false, this);
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	for (const FHitResult& HitResult : HitResults)
	{
		if (HitResult.GetActor() != TargetActor)
		{
			continue;
		}

		AController* MonsterController = GetController();
		if (MonsterController && !MonsterController->LineOfSightTo(TargetActor))
		{
			return false;
		}

		// [공통 데미지 처리 추가] 플레이어 무기와 같은 Data.Damage SetByCaller 경로로 피해를 전달한다.
		return WinterCombat::ApplyDamageEffect(
			AbilitySystemComponent,
			this,
			TargetActor,
			AttackDamageEffect,
			AttackDamage,
			this,
			&HitResult);
	}

	return false;
}

void ABaseMonster::CancelPendingAttack()
{
	PendingAttackTarget = nullptr;
	PendingAttackMontage = nullptr;
}

void ABaseMonster::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (PendingAttackMontage.Get() == Montage)
	{
		// [몬스터 공격 판정 보완] Notify 없이 몽타주가 끝났거나 중단되면 다음 공격이 막히지 않게 정리한다.
		CancelPendingAttack();
	}
}

void ABaseMonster::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!AttributeSet)
	{
		return;
	}

	OnMonsterHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());

	// [성향별 피격 반응] 중립은 전투 상태로, 비선공은 도주 상태로 전환한다.
	if (Data.NewValue > 0.0f && Data.NewValue < Data.OldValue)
	{
		if (Disposition == EMonsterDisposition::Neutral)
		{
			Provoke();
		}
		else if (Disposition == EMonsterDisposition::Passive)
		{
			StartFleeing();
		}
	}

	if (Data.NewValue <= 0.0f)
	{
		Die();
	}
}

void ABaseMonster::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsProvoked = false;
	ResetFleeing();
	CancelPendingAttack();
	if (AbilitySystemComponent)
	{
		// [사망 상태 추가] 투사체나 동일 프레임의 추가 피해가 사망한 몬스터에 적용되지 않게 한다.
		AbilitySystemComponent->AddLooseGameplayTag(WinterGameplayTags::State_Dead);
	}

	// [몬스터 추가] 사망 직후 이동과 충돌을 중지하고 스포너가 다음 검사에서 정리할 수 있도록 수명으로 파괴한다.
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();

	OnMonsterDied.Broadcast();
	OnDeathStarted();

	// [몬스터 추가] SetLifeSpan(0)은 파괴 예약을 취소하므로 0초 설정은 즉시 Destroy로 처리한다.
	if (DestroyDelayAfterDeath <= KINDA_SMALL_NUMBER)
	{
		Destroy();
	}
	else
	{
		SetLifeSpan(DestroyDelayAfterDeath);
	}
}
