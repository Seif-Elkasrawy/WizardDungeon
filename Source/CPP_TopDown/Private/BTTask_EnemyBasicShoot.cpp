// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EnemyBasicShoot.h"
#include "BaseEnemyCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

EBTNodeResult::Type UBTTask_EnemyBasicShoot::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	AAIController* Controller = ownerComp.GetAIOwner();
	ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(Controller->GetPawn());
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(Controller, 0);

	if (Enemy) {
		Enemy->TryFireSpell();
		FVector MuzzleLoc = Enemy->SpawnLocation->GetComponentLocation();

		// 2) Target location: adjust for player eye height
		FVector TargetLocation = Player->GetActorLocation();
		TargetLocation.Z += Player->BaseEyeHeight;  // or a socket offset

		// 3) Compute full 3D rotation
		FRotator AimRotation = (TargetLocation - MuzzleLoc).Rotation();
		// 4) Store it on the enemy for per-frame smoothing
		Enemy->DesiredAimRotation = AimRotation;

		// 5) Fire immediately using the direction vector
		FVector AimDir = AimRotation.Vector();

		if (!AimDir.IsNearlyZero())
		{
			AimDir.Normalize();

			// 3) Compute final velocity
			float speed = Enemy->CharacterStats.shootSpeed;
			FVector Velocity = AimDir * speed;

			Enemy->ShootBullet(Velocity);
		}

		// Enemy->ShootBullet(Controller->GetPawn()->GetActorForwardVector());
		return EBTNodeResult::Succeeded;
	}
	else {
		return EBTNodeResult::Failed;
	}
}
