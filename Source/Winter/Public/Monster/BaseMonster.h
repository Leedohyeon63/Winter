#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "BaseMonster.generated.h"

class UAbilitySystemComponent;
class UBehaviorTree;
class UGameplayEffect;
class UMonsterStatAttributeSet;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

/**
 * 몬스터가 플레이어를 적으로 판단하는 기본 성향이다.
 * Aggressive는 선공, Passive는 비선공, Neutral은 피격 후 적대한다.
 */
UENUM(BlueprintType)
enum class EMonsterDisposition : uint8
{
	Aggressive UMETA(DisplayName = "Aggressive"),
	Passive UMETA(DisplayName = "Passive"),
	Neutral UMETA(DisplayName = "Neutral")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMonsterHealthChangedSignature,
	float,
	CurrentHealth,
	float,
	MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMonsterDiedSignature);

/**
 * 외형 에셋 없이도 스폰·추적·근접 공격·GAS 체력을 시험할 수 있는 기본 몬스터.
 * 실제 몬스터는 이 클래스를 부모로 한 Blueprint에서 메시와 공격 GameplayEffect를 지정한다.
 */
UCLASS()
class WINTER_API ABaseMonster : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseMonster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Monster|AI")
	float GetAggroRange() const { return AggroRange; }

	UFUNCTION(BlueprintPure, Category = "Monster|AI")
	UBehaviorTree* GetBehaviorTreeAsset() const;

	// [몬스터 성향 추가] Blueprint와 BT 보조 로직에서 현재 선공/비선공/중립 유형을 확인한다.
	UFUNCTION(BlueprintPure, Category = "Monster|AI|Disposition")
	EMonsterDisposition GetDisposition() const { return Disposition; }

	UFUNCTION(BlueprintPure, Category = "Monster|AI|Disposition")
	bool IsProvoked() const { return bIsProvoked; }

	// [몬스터 성향 추가] 잘못 구성된 BT에서도 비선공 몬스터가 공격하지 않도록 최종 교전 가능 여부를 제공한다.
	UFUNCTION(BlueprintPure, Category = "Monster|AI|Disposition")
	bool CanEngageTarget(AActor* TargetActor) const;

	// [몬스터 성향 추가] 중립 몬스터를 스크립트나 피격 처리에서 적대 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void Provoke();

	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void ResetProvocation();

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsDead() const { return bIsDead; }

	// [Behavior Tree 변경] BT 공격 Task가 호출하며 성공적으로 GameplayEffect를 적용했는지 반환한다.
	bool TryAttack(AActor* TargetActor);

	UPROPERTY(BlueprintAssignable, Category = "Monster|Events")
	FOnMonsterHealthChangedSignature OnMonsterHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Monster|Events")
	FOnMonsterDiedSignature OnMonsterDied;

protected:
	virtual void BeginPlay() override;

	// [몬스터 추가] 몬스터가 공격 GameplayEffect의 Source가 되도록 자체 ASC를 가진다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Abilities")
	TObjectPtr<UMonsterStatAttributeSet> AttributeSet;

	// [몬스터 추가] 스켈레탈 메시가 준비되기 전 스폰과 이동을 눈으로 확인하기 위한 임시 외형이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	// [몬스터 성향 추가] 몬스터 Blueprint마다 선공/비선공/중립 중 하나를 기본값으로 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Disposition")
	EMonsterDisposition Disposition = EMonsterDisposition::Aggressive;

	// [몬스터 성향 추가] 선공 몬스터가 자동 실행할 Behavior Tree다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Behavior Tree")
	TObjectPtr<UBehaviorTree> AggressiveBehaviorTreeAsset;

	// [몬스터 성향 추가] 비선공 몬스터가 자동 실행할 Behavior Tree다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Behavior Tree")
	TObjectPtr<UBehaviorTree> PassiveBehaviorTreeAsset;

	// [몬스터 성향 추가] 중립 몬스터가 자동 실행할 Behavior Tree다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Behavior Tree")
	TObjectPtr<UBehaviorTree> NeutralBehaviorTreeAsset;

	// [호환 유지] 성향 전용 트리가 비어 있으면 기존에 지정한 Behavior Tree를 폴백으로 실행한다.
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Monster|AI|Behavior Tree",
		meta = (DisplayName = "Fallback Behavior Tree Asset"))
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0.0"))
	float AggroRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0.1"))
	float AttackCooldown = 1.25f;

	// [몬스터 추가] Instant GameplayEffect에서 PlayerStatAttributeSet.Health를 음수로 변경하도록 Blueprint에서 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat")
	TSubclassOf<UGameplayEffect> AttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Death", meta = (ClampMin = "0.0"))
	float DestroyDelayAfterDeath = 1.0f;

	// [몬스터 추가] 나중에 공격 몽타주를 붙일 때 Blueprint에서 시각·음향 연출을 연결할 지점이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|Combat")
	void OnAttackStarted(AActor* TargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|Death")
	void OnDeathStarted();

	// [몬스터 성향 추가] 중립 몬스터가 처음 적대 상태가 될 때 연출을 연결할 Blueprint 지점이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|AI|Disposition")
	void OnProvoked();

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void Die();

	float NextAttackAllowedTime = 0.0f;
	bool bIsDead = false;
	bool bIsProvoked = false;
};

