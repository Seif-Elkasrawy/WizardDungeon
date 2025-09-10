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

	SetInUse(false);
}

// Called every frame
void APooledActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APooledActor::SetInUse(bool bActive)
{
	// If no change, do nothing (prevents re-triggering timers)
	if (bInUse == bActive) return;

	bInUse = bActive;

	if (bInUse)
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		SetActorTickEnabled(true);

		// start auto-return timer if requested
		if (LifeSpan > 0.0f)
		{
			GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
			GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &APooledActor::OnLifeSpanExpired, LifeSpan, false);
		}
	}
	else
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);

		GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);

		// Notify pool that we're available (pool may do additional cleanup)
		if (OwningPool)
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

