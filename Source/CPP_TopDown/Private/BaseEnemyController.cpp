// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseEnemyController.h"
#include "BaseEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

ABaseEnemyController::ABaseEnemyController()
{
    //PrimaryActorTick.bCanEverTick = true;

    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>("PerceptionComp");

    // Create the sight config *as a pointer*
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
    SightConfig->SightRadius = 800.f;
    SightConfig->LoseSightRadius = 1200.f;
    SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->SetMaxAge(5.0f); // How long the sight sense lasts
    

    // Now pass it by reference to ConfigureSense:
    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// Create the hearing config
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>("HearingConfig");
	HearingConfig->HearingRange = 1000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;

	// Pass it by reference to ConfigureSense:
	PerceptionComp->ConfigureSense(*HearingConfig);

	//Create the damage config
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>("DamageConfig");
	DamageConfig->SetMaxAge(5.f); // How long the damage sense lasts
	// Pass it by reference to ConfigureSense:
	PerceptionComp->ConfigureSense(*DamageConfig);

    // Bind your callback
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
        this, &ABaseEnemyController::OnTargetPerceptionUpdated
    );
}

void ABaseEnemyController::BeginPlay()
{
	Super::BeginPlay();
	ABaseEnemyCharacter* enemy = Cast<ABaseEnemyCharacter>(GetPawn());
	if (enemy && enemy->BTAsset) {
		RunBehaviorTree(enemy->BTAsset);
		GetBlackboardComponent()->SetValueAsObject("Player", UGameplayStatics::GetPlayerCharacter(this, 0));
        // Start off in Passive state
        GetBlackboardComponent()->SetValueAsEnum(
            TEXT("State"),
            uint8(EEnemyStates::Passive)
        );
	}

    GetBlackboardComponent()->SetValueAsFloat("LastSeenTime", GetWorld()->GetTimeSeconds() - SightConfig->GetMaxAge() - 1.0f);
	
}

void ABaseEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)  
{  
    if (!GetBlackboardComponent()) return;  

    if (Stimulus.WasSuccessfullySensed() && Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
    {

        if (Actor == GetPawn())
        {
            // Debug
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(
                    -1, 2.f, FColor::Red,
                    TEXT("[AI] Damage sense fired on me!"));

            GetBlackboardComponent()->SetValueAsEnum(
                TEXT("State"),
                uint8(EEnemyStates::Attacking)
            );
		}

	}

    // If we’ve seen the Player and it’s alive, switch to Attacking  
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);  

    if (Actor != Player)  
        return;

    float currentTime = GetWorld()->GetTimeSeconds();
    float LastSeen = GetBlackboardComponent()->GetValueAsFloat(TEXT("LastSeenTime"));

    if (Stimulus.WasSuccessfullySensed())
    {
        // record the world‐space spot you saw the player at
        FVector PlayerLocation = Player->GetActorLocation();

        GetBlackboardComponent()->SetValueAsVector(
            TEXT("LastSeenLocation"),
            PlayerLocation
        );

        // We’ve seen the player RIGHT NOW
        GetBlackboardComponent()->SetValueAsFloat(
            TEXT("LastSeenTime"),
            currentTime
        );

        GetBlackboardComponent()->SetValueAsEnum(
            TEXT("State"),
            uint8(EEnemyStates::Attacking)
        );
    }
    

}
