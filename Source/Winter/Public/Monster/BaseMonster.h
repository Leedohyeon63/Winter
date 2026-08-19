#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "BaseMonster.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
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

	// [비선공 도주 추가] 비선공 몬스터가 피격 후 현재 도주 중인지 BT에서 확인한다.
	UFUNCTION(BlueprintPure, Category = "Monster|AI|Disposition")
	bool IsFleeing() const { return bIsFleeing; }

	// [몬스터 성향 추가] 잘못 구성된 BT에서도 비선공 몬스터가 공격하지 않도록 최종 교전 가능 여부를 제공한다.
	UFUNCTION(BlueprintPure, Category = "Monster|AI|Disposition")
	bool CanEngageTarget(AActor* TargetActor) const;

	// [몬스터 성향 추가] 중립 몬스터를 스크립트나 피격 처리에서 적대 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void Provoke();

	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void ResetProvocation();

	// [비선공 도주 추가] 피격한 플레이어와의 거리 및 도주 제한 시간을 검사하고 만료된 상태를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	bool ShouldContinueFleeingFrom(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void StartFleeing();

	UFUNCTION(BlueprintCallable, Category = "Monster|AI|Disposition")
	void ResetFleeing();

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsDead() const { return bIsDead; }

	// [Behavior Tree 변경] BT 공격 Task가 호출하며 성공적으로 GameplayEffect를 적용했는지 반환한다.
	bool TryAttack(AActor* TargetActor);

	// [몬스터 공격 판정 보완] 공격 몽타주의 AnimNotify가 호출하면 전방 적중 검사를 실행한다.
	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	bool ExecutePendingAttack();

	// [몬스터 공격 판정 보완] 공격 중단·사망 시 남은 판정을 취소한다.
	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	void CancelPendingAttack();

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

	// [비선공 도주 추가] 마지막 피격 후 이 시간이 지나면 도주를 끝낸다. 다시 피격되면 시간이 갱신된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Flee", meta = (ClampMin = "0.0"))
	float FleeDuration = 6.0f;

	// [비선공 도주 추가] 위협과 이 거리 이상 벌어지면 제한 시간이 남아 있어도 도주를 끝낸다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI|Flee", meta = (ClampMin = "0.0"))
	float FleeSafeDistance = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0.1"))
	float AttackCooldown = 1.25f;

	// [공통 데미지 처리 추가] Data.Damage SetByCaller로 플레이어에게 전달할 기본 피해량이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	// [몬스터 공격 판정 보완] 전방 Sphere Sweep의 반지름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat", meta = (ClampMin = "1.0"))
	float AttackRadius = 55.0f;

	// [몬스터 추가] Instant GameplayEffect에서 PlayerStatAttributeSet.Health를 음수로 변경하도록 Blueprint에서 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat")
	TSubclassOf<UGameplayEffect> AttackDamageEffect;

	// [몬스터 공격 판정 보완] 지정하면 C++에서 재생하고 Notify 시점에 실제 피해 판정을 실행할 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat|Animation")
	bool bExecuteAttackOnAnimNotify = false;

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

	// [비선공 도주 추가] 비선공 몬스터가 처음 도주 상태에 진입할 때 연출을 연결할 Blueprint 지점이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|AI|Disposition")
	void OnFleeStarted();

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void Die();

	float NextAttackAllowedTime = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingAttackTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingAttackMontage;

	bool bIsDead = false;
	bool bIsProvoked = false;
	bool bIsFleeing = false;
	float FleeEndTime = 0.0f;
};
