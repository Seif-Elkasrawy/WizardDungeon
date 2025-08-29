// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CPP_TopDownPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Player controller for a top-down perspective game.
 *  Implements point and click based controls
 */
UCLASS(abstract)
class ACPP_TopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Changing to spell 1 Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SelectFirstSpellInput;

	/** Changing to spell 2 Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SelectSecondSpellInput;

	/** Melee Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MeleeInput;
	
	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* FireInput;

	/** Fire Hold Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireHoldInput;

	/** Movement Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MovementInput;

	/** Saved location of the character movement destination */
	FVector CachedDestination;

	/** Time that the click input has been pressed */
	float FollowTime = 0.0f;

public:

	/** Constructor */
	ACPP_TopDownPlayerController();

protected:

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;
	
	/** Gameplay initialization */
	virtual void BeginPlay();

	UPROPERTY(EditAnywhere)
	float speed;

	virtual void Tick(float deltaTime) override;

	void Move(const FInputActionValue& value);

	void SelectFirstSpell(const FInputActionValue& value);

	void SelectSecondSpell(const FInputActionValue& value);

	void FireBullet(const FInputActionValue& value);

	void MeleeAttack(const FInputActionValue& value);

	class ABaseMagicCharacter* PlayerCharacter;

	/** Input handlers */
	void OnInputStarted();
	void OnPlayerStartShooting();
	void OnPlayerStopShooting();
	void OnPlayerStartArc(const FInputActionValue& value);
	void OnPlayerReleaseArc(const FInputActionValue& value);

	void OnPlayerStartMelee();
	void OnPlayerStopMelee();

};


