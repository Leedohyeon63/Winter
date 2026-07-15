// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractSignature, AActor*, Interactor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WINTER_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInteractableComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	// 블루프린트에서 이벤트를 바인딩할 수 있도록 노출
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractSignature OnInteract;

	// 외부(플레이어)에서 호출할 상호작용 함수
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionEnabled() const { return bInteractionEnabled; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FString PromptText = TEXT("상호작용 [F]");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;


};