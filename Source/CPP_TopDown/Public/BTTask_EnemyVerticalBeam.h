// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyVerticalBeam.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UBTTask_EnemyVerticalBeam : public UBTTaskNode
{
	GENERATED_BODY()

	EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory);
	
};
