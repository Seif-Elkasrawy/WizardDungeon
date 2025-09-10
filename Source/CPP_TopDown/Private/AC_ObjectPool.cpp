// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_ObjectPool.h"
#include "PooledActor.h"

// Sets default values for this component's properties
UAC_ObjectPool::UAC_ObjectPool()
{
	PrimaryComponentTick.bCanEverTick = true;

	PoolSize = 20;
	bCanGrow = true;
	GrowthSize = 5;

	// ...
}


void UAC_ObjectPool::BeginPlay()
{
	Super::BeginPlay();

	InitializePool();
	// ...
	
}

void UAC_ObjectPool::InitializePool()
{
	if (!GetWorld()) return;

	// Build list of classes to prefill. Prefer PooledActorClasses if populated; otherwise, fall back to single PooledActorClass.
	TArray<TSubclassOf<APooledActor>> ClassesToInit;
	if (PooledActorClasses.Num() > 0)
	{
		ClassesToInit = PooledActorClasses;
	}
	else if (PooledActorClass)
	{
		ClassesToInit.Add(PooledActorClass);
	}

	// For each class, spawn PoolSize actors
	for (TSubclassOf<APooledActor> Cls : ClassesToInit)
	{
		if (!Cls) continue;

		UClass* Key = Cls.Get();
		if (!PerClassPools.Contains(Key))
		{
			PerClassPools.Add(Key, TArray<TWeakObjectPtr<APooledActor>>());
		}

		GrowPoolForClass(Cls, PoolSize);
	}
}

void UAC_ObjectPool::GrowPoolForClass(TSubclassOf<APooledActor> ForClass, int32 NumToAdd)
{
	if (!GetWorld() || !ForClass) return;

	UClass* Key = ForClass.Get();
	TArray<TWeakObjectPtr<APooledActor>>& PoolArray = PerClassPools.FindOrAdd(Key);

	for (int i = 0; i < NumToAdd; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APooledActor* NewActor = GetWorld()->SpawnActor<APooledActor>(ForClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (NewActor)
		{
			NewActor->OwningPool = this;
			// ensure it's deactivated initially
			NewActor->SetInUse(false);
			PoolArray.Add(TWeakObjectPtr<APooledActor>(NewActor));
		}
	}
}


void UAC_ObjectPool::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

APooledActor* UAC_ObjectPool::GetPooledActor(TSubclassOf<APooledActor> RequestedClass /*= nullptr*/)
{
	// determine which class to use
	TSubclassOf<APooledActor> UseClass = RequestedClass;

	if (!UseClass)
	{
		if (PooledActorClasses.Num() > 0)
		{
			UseClass = PooledActorClasses[0];
		}
		else
		{
			UseClass = PooledActorClass;
		}
	}

	if (!UseClass) return nullptr;

	UClass* Key = UseClass.Get();

	// Ensure we have an entry for this class
	if (!PerClassPools.Contains(Key))
	{
		PerClassPools.Add(Key, TArray<TWeakObjectPtr<APooledActor>>());
		// If we don't have any preallocated instances for this class, spawn initial batch if allowed (PoolSize)
		GrowPoolForClass(UseClass, PoolSize);
	}

	TArray<TWeakObjectPtr<APooledActor>>& PoolArray = PerClassPools.FindOrAdd(Key);

	// Clean up stale/invalid weak pointers (optional but useful)
	for (int32 i = PoolArray.Num() - 1; i >= 0; --i)
	{
		if (!PoolArray[i].IsValid())
		{
			PoolArray.RemoveAtSwap(i);
		}
	}

	// find first available in this class's pool
	for (TWeakObjectPtr<APooledActor>& WeakPtr : PoolArray)
	{
		APooledActor* Actor = WeakPtr.Get();
		if (Actor && !Actor->bInUse)
		{
			Actor->SetInUse(true);
			return Actor;
		}
	}

	// No available actors in this class's pool
	if (bCanGrow)
	{
		GrowPoolForClass(UseClass, GrowthSize);
		// try again after growing
		for (TWeakObjectPtr<APooledActor>& WeakPtr : PoolArray)
		{
			APooledActor* A = WeakPtr.Get();
			if (A && !A->bInUse)
			{
				A->SetInUse(true);
				return A;
			}
		}
	}

	return nullptr; // Pool exhausted and cannot grow
}

void UAC_ObjectPool::ReturnToPool(APooledActor* PooledActor)
{
	if (!IsValid(PooledActor)) return;

	// Find the pool key where the actor should go.
	// Look for first key class where Pooled->IsA(Key). Prefer exact match; fall back to IsA.
	UClass* FoundKey = nullptr;
	for (auto& Pair : PerClassPools)
	{
		UClass* PoolClass = Pair.Key;
		if (!PoolClass) continue;

		if (PooledActor->IsA(PoolClass))
		{
			FoundKey = PoolClass;
			break;
		}
	}

	// If not found, add it under its exact class
	if (!FoundKey)
	{
		UClass* Exact = PooledActor->GetClass();
		TArray<TWeakObjectPtr<APooledActor>>& PoolArray = PerClassPools.FindOrAdd(Exact);
		// avoid duplicates
		if (!PoolArray.Contains(TWeakObjectPtr<APooledActor>(PooledActor)))
		{
			PoolArray.Add(TWeakObjectPtr<APooledActor>(PooledActor));
		}
		PooledActor->OwningPool = this;
		// ensure actor is deactivated
		if (PooledActor->bInUse) PooledActor->SetInUse(false);
		return;
	}

	// if found, ensure it's in that pool array
	TArray<TWeakObjectPtr<APooledActor>>& PoolArray = PerClassPools.FindOrAdd(FoundKey);
	if (!PoolArray.Contains(TWeakObjectPtr<APooledActor>(PooledActor)))
	{
		PoolArray.Add(TWeakObjectPtr<APooledActor>(PooledActor));
	}

	// Deactivate if still marked in use
	if (PooledActor->bInUse)
	{
		PooledActor->SetInUse(false);
	}

	// mark owning pool (defensive)
	PooledActor->OwningPool = this;
}

void UAC_ObjectPool::NotifyActorAvailable(APooledActor* PooledActor)
{
	// Called by APooledActor when it has already run SetInUse(false)
	// keep the pool consistent; we ensure the actor belongs to the pool
	if (!IsValid(PooledActor)) return;

	ReturnToPool(PooledActor);
}

void UAC_ObjectPool::GrowPool(TSubclassOf<APooledActor> ForClass, int32 NumToAdd)
{
	if (!ForClass) return;
	GrowPoolForClass(ForClass, NumToAdd);
}

