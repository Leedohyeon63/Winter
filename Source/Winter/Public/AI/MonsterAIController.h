#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MonsterAIController.generated.h"

UCLASS()
class WINTER_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
    AMonsterAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

	FTimerHandle DecisionTimerHandle;
};

