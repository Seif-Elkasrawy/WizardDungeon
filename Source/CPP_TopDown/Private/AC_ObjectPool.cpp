// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_ObjectPool.h"
#include "PooledActor.h"
#include <Kismet/GameplayStatics.h>

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

	FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("None");
	UE_LOG(LogTemp, Warning, TEXT("[PoolBeginPlay] Pool instance=%p Owner=%s PoolSize=%d NumClasses=%d"),
		this, *OwnerName, PoolSize, PooledActorClasses.Num());

	InitializePool();

	PoolPrewarmReport();
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
			PerClassPools.Add(Key, TArray<APooledActor*>());
		}

		GrowPoolForClass(Cls, PoolSize);
	}

	// After InitializePool spawns for each class, print counts
	//for (auto& Pair : PerClassPools)
	//{
	//	UClass* PoolClass = Pair.Key;
	//	auto& Arr = Pair.Value; // TArray<TWeakObjectPtr<APooledActor>>
	//	int32 ValidCount = 0;
	//	for (auto& W : Arr) if (W.IsValid()) ++ValidCount;
	//	UE_LOG(LogTemp, Warning, TEXT("[Pool] Initialized class %s: pool entries = %d (valid = %d)"),
	//		*GetNameSafe(PoolClass), Arr.Num(), ValidCount);
	//}

}

void UAC_ObjectPool::GrowPoolForClass(TSubclassOf<APooledActor> ForClass, int32 NumToAdd)
{
	if (!GetWorld() || !ForClass) return;

	UClass* Key = ForClass.Get();
	TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(Key);

	for (int i = 0; i < NumToAdd; ++i)
	{
		// Use deferred spawn so we can set OwningPool before BeginPlay runs
		FTransform SpawnTransform = FTransform::Identity;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APooledActor* NewActor = GetWorld()->SpawnActorDeferred<APooledActor>(ForClass, SpawnTransform, SpawnParams.Owner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!NewActor)
		{
			UE_LOG(LogTemp, Error, TEXT("[Pool] Deferred spawn failed for %s"), *GetNameSafe(ForClass.Get()));
			continue;
		}
		// IMPORTANT: set OwningPool BEFORE FinishSpawning so BeginPlay/Construction will see this pointer
		NewActor->OwningPool = this;

		// finish spawn (this will call construction script and BeginPlay soon)
		UGameplayStatics::FinishSpawningActor(NewActor, SpawnTransform);

		NewActor->bInUse = false;
		NewActor->SetActorHiddenInGame(true);
		NewActor->SetActorEnableCollision(false);
		NewActor->SetActorTickEnabled(false);

		PoolArray.Add(NewActor);

		UE_LOG(LogTemp, Log, TEXT("[Pool] Spawned pooled actor '%s' class=%s for pool key=%s"),
			*NewActor->GetName(), *NewActor->GetClass()->GetName(), *ForClass->GetName());
	}
}


void UAC_ObjectPool::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// -- Main GetPooledActor: robust search + fallback
APooledActor* UAC_ObjectPool::GetPooledActor(TSubclassOf<APooledActor> RequestedClass /*= nullptr*/)
{
	TSubclassOf<APooledActor> UseClass = RequestedClass;
	if (!UseClass)
	{
		if (PooledActorClasses.Num() > 0) UseClass = PooledActorClasses[0];
		else UseClass = PooledActorClass;
	}
	if (!UseClass) return nullptr;

	UClass* RequestKey = UseClass.Get();
	UE_LOG(LogTemp, Log, TEXT("[Pool] GetPooledActor requested class=%s"), *GetNameSafe(RequestKey));

	// ensure there's at least an entry array for the requested class
	if (!PerClassPools.Contains(RequestKey))
	{
		PerClassPools.Add(RequestKey, TArray<APooledActor*>());
		GrowPoolForClass(UseClass, PoolSize);
	}

	// First attempt: exact key match
	{
		TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(RequestKey);

		// cleanup invalid pointers
		for (int32 i = PoolArray.Num() - 1; i >= 0; --i)
		{
			if (!IsValid(PoolArray[i]))
			{
				PoolArray.RemoveAtSwap(i);
			}
		}

		// Debug counts
		int32 total = PoolArray.Num();
		int32 freeCount = 0;
		for (APooledActor* A : PoolArray) if (IsValid(A) && !A->bInUse) ++freeCount;
		UE_LOG(LogTemp, Log, TEXT("[Pool] GetPooledActor debug for %s: total=%d free=%d"), *GetNameSafe(RequestKey), total, freeCount);

		for (APooledActor*& Actor : PoolArray)
		{
			if (Actor && !Actor->bInUse)
			{
				Actor->SetInUse(true);
				UE_LOG(LogTemp, Log, TEXT("[Pool] -> returning pooled actor '%s' (bInUse=%d) from class pool %s"),
					*Actor->GetName(), Actor->bInUse, *GetNameSafe(RequestKey));
				return Actor;
			}
		}
	}

	// Second attempt: try compatible pools (subclasses/supertypes)
	for (auto& Pair : PerClassPools)
	{
		UClass* Key = Pair.Key;
		if (Key == RequestKey) continue;
		// If the pool key is a child of requested class or vice versa, it might be usable
		// (tweak the condition depending on how you register classes)
		if (Key->IsChildOf(RequestKey) || RequestKey->IsChildOf(Key))
		{
			TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(Key);
			for (APooledActor* Actor : PoolArray)
			{
				if (Actor && !Actor->bInUse)
				{
					Actor->SetInUse(true);
					UE_LOG(LogTemp, Log, TEXT("[Pool] -> returning pooled actor '%s' (bInUse=%d) from nearby class pool %s"),
						*Actor->GetName(), Actor->bInUse, *GetNameSafe(Key));
					return Actor;
				}
			}
		}
	}

	// Nothing available: grow if allowed
	if (bCanGrow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pool] No free actor for %s; growing pool by %d"), *GetNameSafe(RequestKey), GrowthSize);
		GrowPoolForClass(UseClass, GrowthSize);

		// After growing, try again
		TArray<APooledActor*>& PoolArray2 = PerClassPools.FindOrAdd(RequestKey);
		for (APooledActor*& Actor : PoolArray2)
		{
			if (Actor && !Actor->bInUse)
			{
				Actor->SetInUse(true);
				UE_LOG(LogTemp, Log, TEXT("[Pool] -> grow pooled actor then get spawned '%s' (bInUse=%d) from class pool %s"),
					*Actor->GetName(), Actor->bInUse, *GetNameSafe(RequestKey));
				return Actor;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Pool] Pool exhausted for %s and cannot grow."), *GetNameSafe(RequestKey));
	}

	return nullptr;
}

void UAC_ObjectPool::ReturnToPool(APooledActor* PooledActor)
{
	if (!IsValid(PooledActor)) return;

	// Prefer exact class match
	UClass* Exact = PooledActor->GetClass();
	if (PerClassPools.Contains(Exact))
	{
		TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(Exact);
		if (!PoolArray.Contains(PooledActor))
		{
			PoolArray.Add(PooledActor);
		}
		if (PooledActor->bInUse) PooledActor->SetInUse(false);
		PooledActor->OwningPool = this;
		return;
	}

	// fallback: find a pool key where the actor IsA(key)
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
		TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(Exact);		// avoid duplicates
		if (!PoolArray.Contains(PooledActor))
		{
			PoolArray.Add(PooledActor);
		}
		PooledActor->OwningPool = this;
		// ensure actor is deactivated
		if (PooledActor->bInUse) PooledActor->SetInUse(false);
		return;
	}

	// if found, ensure it's in that pool array
	TArray<APooledActor*>& PoolArray = PerClassPools.FindOrAdd(FoundKey);
	if (!PoolArray.Contains(PooledActor))
	{
		PoolArray.Add(PooledActor);
	}
	// Deactivate if still marked in use
	if (PooledActor->bInUse) PooledActor->SetInUse(false);
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

// -- Utility report function you can call from BP / C++ to see pool state
void UAC_ObjectPool::PoolPrewarmReport()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Pool Status Report ==="));
	for (auto& Pair : PerClassPools)
	{
		UClass* Key = Pair.Key;
		auto& Arr = Pair.Value;
		int32 Valid = 0;
		for (auto& W : Arr) if (IsValid(W)) ++Valid;
		UE_LOG(LogTemp, Warning, TEXT(" Pool for class %s : total=%d valid=%d"), *GetNameSafe(Key), Arr.Num(), Valid);
		for (int32 i = 0; i < Arr.Num(); ++i)
		{
			APooledActor* A = Arr[i];
			UE_LOG(LogTemp, Warning, TEXT("   [%d] %s  valid=%d  bInUse=%d owner=%s"), i,
				A ? *A->GetName() : TEXT("null"),
				A ? 1 : 0,
				A ? (int)A->bInUse : 0,
				A ? *GetNameSafe(A->GetOwner()) : TEXT("none"));
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("=== End Pool Report ==="));
}

