// Fill out your copyright notice in the Description page of Project Settings.


#include "PooledActor.h"
#include "AC_ObjectPool.h"

// Sets default values
APooledActor::APooledActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bInUse = false;
	OwningPool = nullptr;
}

// Called when the game starts or when spawned
void APooledActor::BeginPlay()
{
	Super::BeginPlay();

    // mark that BeginPlay happened so SetInUse(false) will notify the pool only afterwards
    bHasBegunPlay = true;

    // Ensure default state: not in use and deactivated for pooling
    bInUse = false;

    // Hide/disable visuals if created in game (only apply minimal deactivation here; do not notify pool)
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
}

// Called every frame
void APooledActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APooledActor::SetInUse(bool bActive)
{
    UE_LOG(LogTemp, Log, TEXT("[PooledActor] %s SetInUse(%d) Called. OwningPool=%s bHasBegunPlay=%d"),
        *GetName(), (int)bActive, *GetNameSafe(OwningPool), (int)bHasBegunPlay);

    if (bInUse == bActive) return; // no-op if already in that state

    bInUse = bActive;

    if (bInUse)
    {
        SetActorHiddenInGame(false);
        SetActorEnableCollision(true);
        SetActorTickEnabled(true);

        if (LifeSpan > 0.f)
        {
            GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, [this]() { SetInUse(false); }, LifeSpan, false);
        }
        else
        {
            GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
        }
    }
    else
    {

        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        SetActorTickEnabled(false);
        GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);

        if (GetWorld())
        {
            LastReturnedTime = GetWorld()->GetTimeSeconds();
        }

        // Only notify pool if BeginPlay ran (prevents race during construction/deferred spawn)
        if (OwningPool && bHasBegunPlay)
        {
            OwningPool->NotifyActorAvailable(this);
        }
    }
}


void APooledActor::ReturnToPool()
{
	// Ask the pool to accept us back (pool will call SetInUse(false) if needed)
	if (OwningPool)
	{
		OwningPool->ReturnToPool(this);
	}
	else
	{
		// No pool assigned, just deactivate
		SetInUse(false);
	}
}

void APooledActor::OnLifeSpanExpired()
{
	// Auto-return to pool when lifespan expires
	ReturnToPool();
}

