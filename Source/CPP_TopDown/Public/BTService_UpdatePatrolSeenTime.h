// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdatePatrolSeenTime.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UBTService_UpdatePatrolSeenTime : public UBTService
{
	GENERATED_BODY()

	UBTService_UpdatePatrolSeenTime();

	virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector SelfActorKey;

	bool FlagForInitialStart;
	
};
