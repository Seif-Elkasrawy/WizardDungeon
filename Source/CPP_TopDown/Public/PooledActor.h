// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PooledActor.generated.h"

class UAC_ObjectPool;

UCLASS()
class CPP_TOPDOWN_API APooledActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APooledActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// inside APooledActor.h (public or protected area)
	UPROPERTY(VisibleAnywhere, Category = "Pool")
	bool bHasBegunPlay = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	bool bInUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	float LifeSpan;

	// editable tuning parameter
	UPROPERTY(EditAnywhere, Category = "Pool")
	float MinReuseDelay = 0.5f;    // 0.05s (50 ms) — tune to 0.1s if needed

	// runtime bookkeeping
	float LastReturnedTime = -FLT_MAX;

	FTimerHandle LifeSpanTimerHandle;

	/** The pool that owns this actor (set by pool during initialization) */
	UPROPERTY()
	UAC_ObjectPool* OwningPool;

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void SetInUse(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void ReturnToPool();

protected:

	void OnLifeSpanExpired();
};
