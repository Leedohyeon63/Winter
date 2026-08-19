// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace WinterGameplayTags
{
	WINTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_AttackType_Melee);

	WINTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_AttackType_Hitscan);

	WINTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_AttackType_Projectile);

	// [공통 데미지 처리 추가] GameplayEffect에 실제 피해량을 전달하는 SetByCaller 태그다.
	WINTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

	// [사망 상태 추가] 체력이 0이 된 액터의 추가 공격과 피격을 막는 공통 상태 태그다.
	WINTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
}
