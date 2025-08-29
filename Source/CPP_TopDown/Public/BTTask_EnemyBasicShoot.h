// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyBasicShoot.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UBTTask_EnemyBasicShoot : public UBTTaskNode
{
	GENERATED_BODY()

	EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory);
	
};
