// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateDistanceToPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateDistanceToPlayer::UBTService_UpdateDistanceToPlayer()
{
	NodeName = "Update Distance to Player";
}

void UBTService_UpdateDistanceToPlayer::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
	AActor* SelfActor = Cast<AActor>(ownerComp.GetBlackboardComponent()->GetValueAsObject(SelfActorKey.SelectedKeyName));
	AActor* PlayerActor = Cast<AActor>(ownerComp.GetBlackboardComponent()->GetValueAsObject(Player.SelectedKeyName));

	if (SelfActor && PlayerActor) {
		float distance = FVector::Dist(SelfActor->GetActorLocation(), PlayerActor->GetActorLocation());
		ownerComp.GetBlackboardComponent()->SetValueAsFloat(DistanceToPlayer.SelectedKeyName, distance);
	}
}
