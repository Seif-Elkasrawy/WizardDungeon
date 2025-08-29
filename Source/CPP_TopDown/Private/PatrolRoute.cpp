// Fill out your copyright notice in the Description page of Project Settings.


#include "PatrolRoute.h"

// Sets default values
APatrolRoute::APatrolRoute()
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolRoute"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APatrolRoute::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APatrolRoute::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    if (NumPoints < 2) return;

    // compute next index and flip at ends
    NextPoint = CurrentPatrolPoint + PatrolDirection;
    if (NextPoint >= NumPoints)
    {
        PatrolDirection = -1;
        NextPoint = CurrentPatrolPoint + PatrolDirection;
    }
    else if (NextPoint < 0)
    {
        PatrolDirection = 1;
        NextPoint = CurrentPatrolPoint + PatrolDirection;
    }

    // 2) Ask the spline for its world location at that point
    TargetLocation = Spline->GetLocationAtSplinePoint(NextPoint, ESplineCoordinateSpace::World);
}

// Returns whatever was most recently computed in Tick()
FVector APatrolRoute::GetWorldSplineLocation() const
{
    return TargetLocation;
}

void APatrolRoute::AdvancePatrolPoint()
{
    CurrentPatrolPoint = NextPoint;
}


