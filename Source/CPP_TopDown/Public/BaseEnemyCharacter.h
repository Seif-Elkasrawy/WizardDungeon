// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "BaseMagicCharacter.h"
#include "PatrolRoute.h"
#include "VerticalBeamSpell.h" 
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"  
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "HPWidgetBase.h"
#include "BaseEnemyCharacter.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API ABaseEnemyCharacter : public ABaseMagicCharacter
{
	GENERATED_BODY()

public:
	
	ABaseEnemyCharacter();
	
	UPROPERTY(EditDefaultsOnly)
	class UBehaviorTree* BTAsset;

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

	virtual float TakeDamage(
		float DamageCount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	)override;

	/** The spline‐route this enemy will patrol along */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "AI|Patrol")
	APatrolRoute* PatrolRouteActor;

	UPROPERTY(EditDefaultsOnly, Category = "Spells")
	TSubclassOf<AVerticalBeamSpell> VerticalBeamSpellClass;

	USplineComponent* SplineComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthWidgetComponent;

	/** Optional fallback widget class to use if the instance's EnemyHealthWidgetClass is not set */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DefaultEnemyHealthWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> EnemyHealthWidgetClass;

	// runtime pointers
	UPROPERTY()
	UHPWidgetBase* EnemyHPWidgetInstance = nullptr;

	UPROPERTY()
	UUserWidget* EnemyRawWidgetInstance = nullptr;

public:

	/** Spawn a vertical beam at or above the given target actor.
	 *  If bAttachToTarget == true, beam is attached to the target's root component.
	 */
	UFUNCTION(BlueprintCallable, Category = "FX")
	void SpawnVerticalBeamAtActor(AActor* TargetActor, float Duration = 2.0f, float BeamScale = 0.1f);
};
