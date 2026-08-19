#include "Components/WeaponManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Actor/WeaponProjectile.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/WinterCombat.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "WinterGameplayTags.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UPlayerInventoryComponent>()
		: nullptr;

	if (InventoryComponent)
	{
		// [웨폰 매니저 추가] 장착/해제와 레벨 이동 복원 후 활성 무기를 자동으로 다시 확인한다.
		InventoryComponent->OnInventoryChanged.AddDynamic(
			this,
			&UWeaponManagerComponent::HandleInventoryChanged);
	}

	RefreshActiveWeapon();
}

void UWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// [공격 대기 안정성 보완] 레벨 이동이나 액터 제거 중 남은 AnimNotify 공격을 폐기한다.
	CancelPendingAttack();

	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(
			this,
			&UWeaponManagerComponent::HandleInventoryChanged);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWeaponManagerComponent::StartAttack()
{
	RefreshActiveWeapon();

	UItemDefinitionDataAsset* WeaponDefinition = GetActiveWeapon();
	UWorld* World = GetWorld();
	IAbilitySystemInterface* OwnerAbilityInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* OwnerAbilitySystem = OwnerAbilityInterface
		? OwnerAbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!WeaponDefinition
		|| !World
		|| !OwnerAbilitySystem
		|| OwnerAbilitySystem->HasMatchingGameplayTag(WinterGameplayTags::State_Dead)
		|| !WeaponDefinition->AttackTypeTag.IsValid()
		|| !WeaponDefinition->AttackDamageEffect
		|| WeaponDefinition->AttackDamage <= 0.0f
		|| PendingAttackWeapon)
	{
		return false;
	}

	const FGameplayTag& AttackType = WeaponDefinition->AttackTypeTag;
	const bool bIsMelee = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Melee);
	const bool bIsHitscan = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Hitscan);
	const bool bIsProjectile = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Projectile);
	if ((!bIsMelee && !bIsHitscan && !bIsProjectile)
		|| (bIsProjectile && !WeaponDefinition->ProjectileClass))
	{
		// [웨폰 매니저 추가] 잘못 설정된 무기는 쿨다운과 몽타주가 시작되기 전에 거부한다.
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime < NextAttackAllowedTime)
	{
		return false;
	}

	PendingAttackWeapon = WeaponDefinition;
	NextAttackAllowedTime = CurrentTime + FMath::Max(0.0f, WeaponDefinition->AttackCooldown);

	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	const float MontageDuration = CharacterOwner && WeaponDefinition->AttackMontage
		? CharacterOwner->PlayAnimMontage(WeaponDefinition->AttackMontage)
		: 0.0f;

	// [웨폰 매니저 추가] 유효한 몽타주와 Notify 설정이 모두 있을 때만 판정을 대기한다.
	if (WeaponDefinition->bExecuteAttackOnAnimNotify && MontageDuration > 0.0f)
	{
		// [공격 대기 안정성 보완] Notify가 실행되지 않아도 몽타주 종료 시 대기 상태가 자동으로 해제된다.
		PendingAttackMontage = WeaponDefinition->AttackMontage;
		if (CharacterOwner && CharacterOwner->GetMesh())
		{
			if (UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance())
			{
				FOnMontageEnded MontageEndedDelegate;
				MontageEndedDelegate.BindUObject(this, &UWeaponManagerComponent::HandleAttackMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, PendingAttackMontage.Get());
			}
		}
		return true;
	}

	return ExecutePendingAttack();
}

bool UWeaponManagerComponent::ExecutePendingAttack()
{
	UItemDefinitionDataAsset* WeaponDefinition = PendingAttackWeapon.Get();
	PendingAttackWeapon = nullptr;
	PendingAttackMontage = nullptr;

	if (!WeaponDefinition)
	{
		return false;
	}

	const FGameplayTag& AttackType = WeaponDefinition->AttackTypeTag;
	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Melee))
	{
		return ExecuteMeleeAttack(WeaponDefinition);
	}

	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Hitscan))
	{
		return ExecuteHitscanAttack(WeaponDefinition);
	}

	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Projectile))
	{
		return ExecuteProjectileAttack(WeaponDefinition);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("WeaponManager: 지원하지 않는 AttackTypeTag입니다: %s"),
		*AttackType.ToString());
	return false;
}

void UWeaponManagerComponent::CancelPendingAttack()
{
	// [공격 대기 안정성 보완] 이미 실행된 Notify와 이후 MontageEnded가 겹쳐도 포인터 초기화만 수행한다.
	PendingAttackWeapon = nullptr;
	PendingAttackMontage = nullptr;
}

void UWeaponManagerComponent::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (PendingAttackMontage.Get() == Montage)
	{
		CancelPendingAttack();
	}
}

bool UWeaponManagerComponent::SwitchWeapon()
{
	const EEquipmentSlot OtherSlot = ActiveWeaponSlot == EEquipmentSlot::PrimaryWeapon
		? EEquipmentSlot::SecondaryWeapon
		: EEquipmentSlot::PrimaryWeapon;

	return SetActiveWeaponSlot(OtherSlot);
}

bool UWeaponManagerComponent::SetActiveWeaponSlot(EEquipmentSlot NewSlot)
{
	if (!InventoryComponent
		|| (NewSlot != EEquipmentSlot::PrimaryWeapon
			&& NewSlot != EEquipmentSlot::SecondaryWeapon)
		|| !InventoryComponent->GetEquippedItem(NewSlot))
	{
		return false;
	}

	if (ActiveWeaponSlot == NewSlot)
	{
		return true;
	}

	// [웨폰 매니저 추가] 무기 전환 시 이전 무기의 대기 중 AnimNotify 공격이 실행되지 않도록 취소한다.
	CancelPendingAttack();
	ActiveWeaponSlot = NewSlot;
	RefreshActiveWeapon();
	return true;
}

UItemDefinitionDataAsset* UWeaponManagerComponent::GetActiveWeapon() const
{
	return InventoryComponent
		? InventoryComponent->GetEquippedItem(ActiveWeaponSlot)
		: nullptr;
}

void UWeaponManagerComponent::HandleInventoryChanged()
{
	RefreshActiveWeapon();
}

void UWeaponManagerComponent::RefreshActiveWeapon()
{
	if (!InventoryComponent)
	{
		return;
	}

	UItemDefinitionDataAsset* CurrentWeapon = InventoryComponent->GetEquippedItem(ActiveWeaponSlot);
	if (!CurrentWeapon)
	{
		const EEquipmentSlot OtherSlot = ActiveWeaponSlot == EEquipmentSlot::PrimaryWeapon
			? EEquipmentSlot::SecondaryWeapon
			: EEquipmentSlot::PrimaryWeapon;
		if (InventoryComponent->GetEquippedItem(OtherSlot))
		{
			ActiveWeaponSlot = OtherSlot;
			CurrentWeapon = InventoryComponent->GetEquippedItem(OtherSlot);
		}
	}

	if (PendingAttackWeapon && PendingAttackWeapon != CurrentWeapon)
	{
		CancelPendingAttack();
	}

	if (LastActiveWeapon != CurrentWeapon)
	{
		LastActiveWeapon = CurrentWeapon;
		OnActiveWeaponChanged.Broadcast(ActiveWeaponSlot, CurrentWeapon);
	}
}

bool UWeaponManagerComponent::ExecuteMeleeAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!CharacterOwner || !World || !WeaponDefinition || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	const FVector Forward = CharacterOwner->GetActorForwardVector();
	const FVector Start = CharacterOwner->GetActorLocation()
		+ FVector::UpVector * CharacterOwner->GetSimpleCollisionHalfHeight() * 0.5f;
	const FVector End = Start + Forward * FMath::Max(0.0f, WeaponDefinition->AttackRange);
	const float Radius = FMath::Max(1.0f, WeaponDefinition->AttackRadius);

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponMeleeSweep), false, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if (bDrawDebugTraces)
	{
		DrawDebugLine(World, Start, End, FColor::Orange, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(World, Start, Radius, 16, FColor::Orange, false, 1.0f);
		DrawDebugSphere(World, End, Radius, 16, FColor::Orange, false, 1.0f);
	}

	TSet<AActor*> DamagedActors;
	bool bAppliedAnyDamage = false;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || DamagedActors.Contains(TargetActor))
		{
			continue;
		}

		AController* OwnerController = CharacterOwner->GetController();
		if (OwnerController && !OwnerController->LineOfSightTo(TargetActor))
		{
			continue;
		}

		DamagedActors.Add(TargetActor);
		bAppliedAnyDamage |= ApplyDamageEffect(TargetActor, WeaponDefinition, &HitResult);
	}

	return bAppliedAnyDamage;
}

bool UWeaponManagerComponent::ExecuteHitscanAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!CharacterOwner || !World || !WeaponDefinition || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	FVector CameraStart = CharacterOwner->GetActorLocation();
	FRotator ViewRotation = CharacterOwner->GetActorRotation();
	if (AController* OwnerController = CharacterOwner->GetController())
	{
		OwnerController->GetPlayerViewPoint(CameraStart, ViewRotation);
	}

	const FVector CameraEnd = CameraStart
		+ ViewRotation.Vector() * FMath::Max(0.0f, WeaponDefinition->AttackRange);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponHitscan), true, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

	FHitResult CameraHit;
	const bool bCameraHit = World->LineTraceSingleByChannel(
		CameraHit,
		CameraStart,
		CameraEnd,
		ECC_Visibility,
		QueryParams);
	const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : CameraEnd;

	FVector TraceStart = CameraStart;
	if (USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh())
	{
		if (CharacterMesh->DoesSocketExist(WeaponDefinition->ProjectileSocketName))
		{
			TraceStart = CharacterMesh->GetSocketLocation(WeaponDefinition->ProjectileSocketName);
		}
	}

	// [히트스캔 판정 보완] 카메라 조준점까지 총구에서 다시 검사해 총구 앞 벽을 관통하지 못하게 한다.
	FHitResult FinalHit;
	const bool bFinalHit = World->LineTraceSingleByChannel(
		FinalHit,
		TraceStart,
		AimPoint,
		ECC_Visibility,
		QueryParams);

	if (bDrawDebugTraces)
	{
		DrawDebugLine(
			World,
			TraceStart,
			bFinalHit ? FinalHit.ImpactPoint : AimPoint,
			bFinalHit ? FColor::Green : FColor::Red,
			false,
			1.0f,
			0,
			2.0f);
	}

	return bFinalHit && ApplyDamageEffect(FinalHit.GetActor(), WeaponDefinition, &FinalHit);
}

bool UWeaponManagerComponent::ExecuteProjectileAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!CharacterOwner
		|| !World
		|| !WeaponDefinition
		|| !WeaponDefinition->ProjectileClass
		|| !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	FVector SpawnLocation = CharacterOwner->GetActorLocation()
		+ CharacterOwner->GetActorForwardVector() * 100.0f;
	FRotator SpawnRotation = CharacterOwner->GetActorRotation();

	if (USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh())
	{
		if (CharacterMesh->DoesSocketExist(WeaponDefinition->ProjectileSocketName))
		{
			SpawnLocation = CharacterMesh->GetSocketLocation(WeaponDefinition->ProjectileSocketName);
		}
	}

	FVector AimPoint = SpawnLocation + SpawnRotation.Vector() * WeaponDefinition->AttackRange;
	if (AController* OwnerController = CharacterOwner->GetController())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FVector ViewEnd = ViewLocation
			+ ViewRotation.Vector() * FMath::Max(0.0f, WeaponDefinition->AttackRange);

		FCollisionQueryParams AimQueryParams(SCENE_QUERY_STAT(WeaponProjectileAim), true, CharacterOwner);
		AimQueryParams.AddIgnoredActor(CharacterOwner);
		FHitResult AimHit;
		AimPoint = World->LineTraceSingleByChannel(
			AimHit,
			ViewLocation,
			ViewEnd,
			ECC_Visibility,
			AimQueryParams)
			? AimHit.ImpactPoint
			: ViewEnd;
	}

	// [투사체 조준 보완] 카메라와 평행하게 쏘지 않고 실제 총구에서 조준점을 향하도록 회전한다.
	const FVector AimDirection = AimPoint - SpawnLocation;
	if (!AimDirection.IsNearlyZero())
	{
		SpawnRotation = AimDirection.Rotation();
	}

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	// [투사체 안정성 보완] 충돌과 이동이 시작되기 전에 공격 정보를 모두 전달하기 위해 지연 생성한다.
	AWeaponProjectile* Projectile = World->SpawnActorDeferred<AWeaponProjectile>(
		WeaponDefinition->ProjectileClass,
		SpawnTransform,
		CharacterOwner,
		CharacterOwner,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile)
	{
		return false;
	}

	IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(CharacterOwner);
	Projectile->InitializeProjectile(
		CharacterOwner,
		AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr,
		WeaponDefinition->AttackDamageEffect,
		WeaponDefinition,
		WeaponDefinition->AttackDamage,
		WeaponDefinition->ProjectileSpeed,
		WeaponDefinition->ProjectileLifeSeconds);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	return true;
}

bool UWeaponManagerComponent::ApplyDamageEffect(
	AActor* TargetActor,
	UItemDefinitionDataAsset* WeaponDefinition,
	const FHitResult* HitResult) const
{
	if (!IsValid(TargetActor) || TargetActor == GetOwner() || !WeaponDefinition)
	{
		return false;
	}

	IAbilitySystemInterface* SourceInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* SourceAbilitySystem = SourceInterface
		? SourceInterface->GetAbilitySystemComponent()
		: nullptr;

	if (!SourceAbilitySystem || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	// [공통 데미지 처리 추가] 세 공격 타입 모두 같은 SetByCaller 피해 전달 경로를 사용한다.
	return WinterCombat::ApplyDamageEffect(
		SourceAbilitySystem,
		GetOwner(),
		TargetActor,
		WeaponDefinition->AttackDamageEffect,
		WeaponDefinition->AttackDamage,
		WeaponDefinition,
		HitResult);
}
