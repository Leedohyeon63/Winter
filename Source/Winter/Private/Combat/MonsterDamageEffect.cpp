#include "Combat/MonsterDamageEffect.h"

#include "Attribute/MonsterStatAttributeSet.h"
#include "WinterGameplayTags.h"

UMonsterDamageEffect::UMonsterDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UMonsterStatAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = WinterGameplayTags::Data_Damage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
}
