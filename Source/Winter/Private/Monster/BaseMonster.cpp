#include "Monster/BaseMonster.h"
#include "AI/MonsterAIController.h"
#include "AbilitySystemComponent.h"
#include "Attribute/MonsterStatAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"

ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent =
		CreateDefaultSubobject<UAbilitySystemComponent>(
			TEXT("AbilitySystemComponent"));

	AbilitySystemComponent->SetIsReplicated(false);

	AttributeSet =
		CreateDefaultSubobject<UMonsterStatAttributeSet>(
			TEXT("AttributeSet"));

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	AIControllerClass = AMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

bool ABaseMonster::TryAttack(AActor* TargetActor)
{
    if (bIsDead
        || !IsValid(TargetActor)
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

    const float DistanceSquared = FVector::DistSquared(
        GetActorLocation(),
        TargetActor->GetActorLocation());

    if (DistanceSquared > FMath::Square(AttackRange))
    {
        return false;
    }

    IAbilitySystemInterface* TargetAbilityInterface =
        Cast<IAbilitySystemInterface>(TargetActor);

    UAbilitySystemComponent* TargetAbilitySystem =
        TargetAbilityInterface
        ? TargetAbilityInterface->GetAbilitySystemComponent()
        : nullptr;

    if (!TargetAbilitySystem)
    {
        return false;
    }

    FGameplayEffectContextHandle EffectContext =
        AbilitySystemComponent->MakeEffectContext();

    EffectContext.AddInstigator(this, this);
    EffectContext.AddSourceObject(this);

    const FGameplayEffectSpecHandle SpecHandle =
        AbilitySystemComponent->MakeOutgoingSpec(
            AttackDamageEffect,
            1.0f,
            EffectContext);

    if (!SpecHandle.IsValid())
    {
        return false;
    }

    const FVector ToTarget =
        TargetActor->GetActorLocation() - GetActorLocation();

    SetActorRotation(
        FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));

    OnAttackStarted(TargetActor);

    TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(
        *SpecHandle.Data.Get());

    NextAttackAllowedTime = CurrentTime + AttackCooldown;
    return true;
}

