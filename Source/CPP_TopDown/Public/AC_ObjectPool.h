// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_ObjectPool.generated.h"

class APooledActor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CPP_TOPDOWN_API UAC_ObjectPool : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAC_ObjectPool();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	TSubclassOf<class APooledActor> PooledActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	TArray< TSubclassOf<class APooledActor>> PooledActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (ClampMin = "0"))
	int32 PoolSize = 20;

	UPROPERTY()
	TArray<APooledActor*> ObjectPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	bool bCanGrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (EditCondition = "bCanGrow"))
	int32 GrowthSize = 5;

	//TMap<UClass*, TArray<TWeakObjectPtr<APooledActor>>> PerClassPools;
	TMap<UClass*, TArray<APooledActor*>> PerClassPools;

	UFUNCTION()
	void InitializePool();

	/** Helper to spawn n instances of a given class and add to PerClassPools[class] */
	void GrowPoolForClass(TSubclassOf<APooledActor> ForClass, int32 NumToAdd);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	APooledActor* GetPooledActor(TSubclassOf<APooledActor> RequestedClass = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReturnToPool(APooledActor* PooledActor);

	void NotifyActorAvailable(APooledActor* PooledActor);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void GrowPool(TSubclassOf<APooledActor> ForClass, int32 NumToAdd);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void PoolPrewarmReport();
};
