// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseEnemyCharacter.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyBasicMelee.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UBTTask_EnemyBasicMelee : public UBTTaskNode
{
	GENERATED_BODY()
	
public:

	UBTTask_EnemyBasicMelee();

	EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:

	UPROPERTY()
	ABaseEnemyCharacter* CachedEnemy;

	UPROPERTY()	
	UBehaviorTreeComponent* CachedOwnerComp;

	UFUNCTION()
	void HandleMeleeFinished(bool bIsAttacking);

	bool bIsWaitingForMelee;

};
