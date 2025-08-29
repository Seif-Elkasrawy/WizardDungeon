// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdatePatrolSeenTime.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BaseEnemyController.h"
#include "Kismet/GameplayStatics.h"

UBTService_UpdatePatrolSeenTime::UBTService_UpdatePatrolSeenTime()
{
	NodeName = "Update Patrol Seen Time";
    FlagForInitialStart = true;
}

void UBTService_UpdatePatrolSeenTime::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)  
{  
    AActor* SelfActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(SelfActorKey.SelectedKeyName));  
    ABaseEnemyController* EnemyController = Cast<ABaseEnemyController>(OwnerComp.GetAIOwner()); 
  
    float MaxAge = EnemyController->SightConfig
        ? EnemyController->SightConfig->GetMaxAge()
        : 5.0f;
    float CurrentTime = OwnerComp.GetWorld()->GetTimeSeconds();
    const uint8 State = OwnerComp.GetBlackboardComponent()->GetValueAsEnum(TEXT("State"));

    bool bCanSee = false;
    FAIStimulus Stimulus;
    if (SelfActor && EnemyController) {
        if (Stimulus.WasSuccessfullySensed()) bCanSee = true;
    }

    if (bCanSee) FlagForInitialStart = false;

    if (FlagForInitialStart) {
        OwnerComp.GetBlackboardComponent()->SetValueAsFloat(TEXT("TimeSinceLastSeen"), CurrentTime + 5);
	}

    if (bCanSee) {
        OwnerComp.GetBlackboardComponent()->SetValueAsFloat(TEXT("LastSeenTime"), CurrentTime);
    }

    float LastSeen = OwnerComp.GetBlackboardComponent()->GetValueAsFloat("LastSeenTime");
	float ElapsedSince = CurrentTime - LastSeen;
    OwnerComp.GetBlackboardComponent()->SetValueAsFloat("TimeSinceLastSeen", ElapsedSince);

    if (State == uint8(EEnemyStates::Attacking) && ElapsedSince >= EnemyController->SightConfig->GetMaxAge())
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
                TEXT(">>> Stimulus aged out: switching to Passive"));

        // Lost sight—maybe go back to Passive  
        OwnerComp.GetBlackboardComponent()->SetValueAsEnum(
            TEXT("State"),
            uint8(EEnemyStates::Passive)
        );
    }

}

