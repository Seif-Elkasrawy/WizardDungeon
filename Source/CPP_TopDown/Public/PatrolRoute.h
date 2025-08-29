// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "PatrolRoute.generated.h"

UCLASS()
class CPP_TOPDOWN_API APatrolRoute : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APatrolRoute();

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FVector GetWorldSplineLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	float GetAcceptanceRadius() const { return AcceptanceRadius; }

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	int32 GetCurrentPatrolPoint() const { return CurrentPatrolPoint; }

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	int32 GetNextPoint() const { return NextPoint; }

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void AdvancePatrolPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	USplineComponent* Spline;

	// Which spline‐point we’ve just reached
	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	int32 CurrentPatrolPoint = 0;

	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	int32 NextPoint;

	// +1 when going forward, –1 when going backward
	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	int32 PatrolDirection = 1;

	// How close to a point counts as “reached”
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptanceRadius = 10.f;

	// For debugging / external reads
	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	FVector TargetLocation = FVector::ZeroVector;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
