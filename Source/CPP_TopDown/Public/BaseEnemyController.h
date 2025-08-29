// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "EEnemyStates.h"
#include "BaseEnemyController.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API ABaseEnemyController : public AAIController
{
	GENERATED_BODY()

public:

	ABaseEnemyController();

protected:

	virtual void BeginPlay();

	// Perception
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* PerceptionComp;



	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

public:

	UAISenseConfig_Sight* SightConfig;
	UAISenseConfig_Hearing* HearingConfig;
	UAISenseConfig_Damage* DamageConfig;
	
};
