// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryContext_AttackTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UEnvQueryContext_AttackTarget::ProvideSingleActor(UObject* QuerierObject, AActor*& ResultingActor) const
{
    ResultingActor = nullptr;
    if (!QuerierObject) return;

    // 1) If Querier is an AIController, read its blackboard
    if (const AAIController* AICon = Cast<AAIController>(QuerierObject))
    {
        if (const UBlackboardComponent* BB = AICon->GetBlackboardComponent())
        {
            UObject* Obj = BB->GetValueAsObject(TEXT("Player"));
            ResultingActor = Cast<AActor>(Obj);
            if (ResultingActor) return;
        }
        // fallback: the controlled pawn might expose the target variable
        if (APawn* Pawn = AICon->GetPawn())
        {
            // e.g. ABCustomEnemy* E = Cast<ABCustomEnemy>(Pawn); if has variable, use it
        }
    }

    // 2) If Querier is a Pawn/Character directly, try to read stored variable
    if (APawn* PawnQuerier = Cast<APawn>(QuerierObject))
    {
        // If your Pawn has a stored TargetActor property:
        // ResultingActor = PawnQuerier->FindComponentByClass<...> or cast and read a variable
    }

    // 3) As ultimate fallback, return the player
    if (!ResultingActor && GetWorld())
    {
        ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        ResultingActor = PlayerChar; // implicit upcast ACharacter* -> AActor*
    }
}
