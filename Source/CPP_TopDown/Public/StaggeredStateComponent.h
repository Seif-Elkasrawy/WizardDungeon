// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaggeredStateComponent.generated.h"

class ABaseMagicCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CPP_TOPDOWN_API UStaggeredStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStaggeredStateComponent();

	/** Enter stagger for Duration seconds (Duration <=0 means indefinite until ExitStagger called). */
	UFUNCTION(BlueprintCallable, Category = "Stagger")
	void EnterStagger(float Duration);

	/** Exit stagger state. */
	UFUNCTION(BlueprintCallable, Category = "Stagger")
	void ExitStagger();

	UFUNCTION(BlueprintCallable, Category = "Stagger")
	bool IsStaggered() const { return bIsStaggered; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY()
	ABaseMagicCharacter* OwnerCharacter;

	FTimerHandle StaggerTimerHandle;

	bool bIsStaggered;

	/** Internal helper to apply the "block" effects (disable movement, input, actions). */
	void ApplyBlock();

	/** Internal helper to remove the "blocking" of effects and restore previous state. */
	void RemoveBlock();

	/** Keep track if we actually disabled input/movement so we don't re-enable something we didn't disable. */
	bool bDisabledInput;
	bool bDisabledMovement;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
