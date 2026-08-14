#include "Monster/BaseMonster.h"

#include "AI/MonsterAIController.h"
#include "AbilitySystemComponent.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"

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

	IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetAbilitySystem = TargetAbilityInterface
		? TargetAbilityInterface->GetAbilitySystemComponent()
		: nullptr;

	if (!TargetAbilitySystem)
	{
		return false;
	}

	// [몬스터 추가] 몬스터 ASC에서 Spec을 만들어 플레이어 ASC에 적용하므로 Instigator와 Source가 보존된다.
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddInstigator(this, this);
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(AttackDamageEffect, 1.0f, EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));

	OnAttackStarted(TargetActor);
	TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	NextAttackAllowedTime = CurrentTime + AttackCooldown;
	return true;
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
