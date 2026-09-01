#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelPortal.generated.h"

class UArrowComponent;
class UInteractableComponent;
class UPortalDefinitionDataAsset;
class USphereComponent;
class UStaticMeshComponent;

/** F키 상호작용으로 지정 레벨의 상대 포탈 위치로 이동시키는 포탈. */
UCLASS()
class WINTER_API ALevelPortal : public AActor
{
	GENERATED_BODY()

public:
	ALevelPortal();

	UFUNCTION(BlueprintPure, Category = "Portal")
	FName GetPortalId() const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	FTransform GetArrivalTransform() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> ArrivalPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<UPortalDefinitionDataAsset> PortalDefinition;

private:
	UFUNCTION()
	void HandleInteract(AActor* Interactor);

	void RefreshInteractionState();
};
