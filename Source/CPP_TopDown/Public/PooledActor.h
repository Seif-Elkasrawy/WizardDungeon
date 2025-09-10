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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	bool bInUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	float LifeSpan;

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
