// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateDistanceToPlayer.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UBTService_UpdateDistanceToPlayer : public UBTService
{
	GENERATED_BODY()

	UBTService_UpdateDistanceToPlayer();

	virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector SelfActorKey;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector Player;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector DistanceToPlayer;
	
};
