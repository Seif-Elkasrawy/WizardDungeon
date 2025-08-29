// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyBasicMelee.h"
#include "BaseEnemyCharacter.h"
#include "AIController.h"
#include "TimerManager.h"

UBTTask_EnemyBasicMelee::UBTTask_EnemyBasicMelee()
{
	NodeName = "Enemy Basic Melee";
	bNotifyTick = true;
	bCreateNodeInstance = true;   
}

EBTNodeResult::Type UBTTask_EnemyBasicMelee::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* controller = OwnerComp.GetAIOwner();
	CachedEnemy = Cast<ABaseEnemyCharacter>(controller ? controller->GetPawn() : nullptr);

	if (CachedEnemy)
	{
		CachedOwnerComp = &OwnerComp;
		bIsWaitingForMelee = true;

		CachedEnemy->OnMeleeFinished.AddDynamic(this, &UBTTask_EnemyBasicMelee::HandleMeleeFinished);

		CachedEnemy->MeleeAttack();

		UE_LOG(LogTemp, Log, TEXT("[%s] Starting BasicMeleeAttack"), *CachedEnemy->GetName());


		return EBTNodeResult::InProgress; // Wait until animation finishes
	}

	return EBTNodeResult::Failed;
}

void UBTTask_EnemyBasicMelee::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
}

void UBTTask_EnemyBasicMelee::HandleMeleeFinished(bool bIsAttacking)
{
	if (!bIsAttacking && CachedOwnerComp && CachedEnemy)
	{
		CachedEnemy->OnMeleeFinished.RemoveDynamic(this, &UBTTask_EnemyBasicMelee::HandleMeleeFinished);
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}
}

