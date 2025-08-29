// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyVerticalBeam.h"
#include "BaseEnemyCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"


EBTNodeResult::Type UBTTask_EnemyVerticalBeam::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	AAIController* Controller = ownerComp.GetAIOwner();
	ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(Controller->GetPawn());
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(Controller, 0);

	if (Enemy) {

		if (Player)
		{
			// spawn above player by 900 units, attach = false, duration = 2s
			Enemy->SpawnVerticalBeamAtActor(Player, 2.0f, false, 0.1f);
		}

		return EBTNodeResult::Succeeded;
	}
	else {
		return EBTNodeResult::Failed;
	}
}

