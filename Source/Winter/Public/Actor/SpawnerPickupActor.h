#pragma once

#include "CoreMinimal.h"
#include "Actor/PickupActor.h"
#include "SpawnerPickupActor.generated.h"

/** Compatibility class for existing assets. Spawn state is handled by PickupActor. */
UCLASS()
class WINTER_API ASpawnerPickupActor : public APickupActor
{
	GENERATED_BODY()
};
