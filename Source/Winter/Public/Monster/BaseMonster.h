#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "BaseMonster.generated.h"

class UAbilitySystemComponent;
class UBehaviorTree;
class UGameplayEffect;
class UMonsterStatAttributeSet;


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
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsDead() const { return bIsDead; }

	bool TryAttack(AActor* TargetActor);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Abilities")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Abilities")
    TObjectPtr<UMonsterStatAttributeSet> AttributeSet;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI",
        meta = (ClampMin = "0.0"))
    float AggroRange = 2000.0f;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.0"))
    float AttackRange = 160.0f;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.1"))
    float AttackCooldown = 1.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Combat")
    TSubclassOf<UGameplayEffect> AttackDamageEffect;

    UFUNCTION(BlueprintImplementableEvent, Category = "Monster|Combat")
    void OnAttackStarted(AActor* TargetActor);

private:
    float NextAttackAllowedTime = 0.0f;
    bool bIsDead = false;
};

