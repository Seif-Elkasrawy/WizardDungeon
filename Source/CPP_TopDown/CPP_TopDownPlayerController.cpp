// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPP_TopDownPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "CPP_TopDownCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Perception/AISense_Hearing.h"
#include "BaseMagicCharacter.h"
#include "BasePlayerCharacter.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ACPP_TopDownPlayerController::ACPP_TopDownPlayerController()
{

	// configure the 
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void ACPP_TopDownPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	PlayerCharacter = Cast<ABaseMagicCharacter>(GetPawn());
}

void ACPP_TopDownPlayerController::Tick(float deltaTime)
{

}

void ACPP_TopDownPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Movement Bindings
		EnhancedInputComponent->BindAction(MovementInput, ETriggerEvent::Triggered, this, &ACPP_TopDownPlayerController::Move);
		// Spell Selection Bindings
		EnhancedInputComponent->BindAction(SelectFirstSpellInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::SelectFirstSpell);
		EnhancedInputComponent->BindAction(SelectSecondSpellInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::SelectSecondSpell);
		// Spell Casting Bindings
		EnhancedInputComponent->BindAction(FireInput, ETriggerEvent::Triggered, this, &ACPP_TopDownPlayerController::FireBullet);
		EnhancedInputComponent->BindAction(FireInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::OnPlayerStartShooting);
		EnhancedInputComponent->BindAction(FireInput, ETriggerEvent::Completed, this, &ACPP_TopDownPlayerController::OnPlayerStopShooting);
		EnhancedInputComponent->BindAction(FireHoldInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::OnPlayerStartArc);
		EnhancedInputComponent->BindAction(FireHoldInput, ETriggerEvent::Completed, this, &ACPP_TopDownPlayerController::OnPlayerReleaseArc);
		// Melee Bindings
		EnhancedInputComponent->BindAction(MeleeInput, ETriggerEvent::Triggered, this, &ACPP_TopDownPlayerController::MeleeAttack);
		EnhancedInputComponent->BindAction(MeleeInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::OnPlayerStartMelee);
		EnhancedInputComponent->BindAction(MeleeInput, ETriggerEvent::Completed, this, &ACPP_TopDownPlayerController::OnPlayerStopMelee);
		// Dodge Bindings
		//EnhancedInputComponent->BindAction(DodgeInput, ETriggerEvent::Triggered, this, &ACPP_TopDownPlayerController::Dodge);
		EnhancedInputComponent->BindAction(DodgeInput, ETriggerEvent::Started, this, &ACPP_TopDownPlayerController::OnPlayerStartDodge);
		EnhancedInputComponent->BindAction(DodgeInput, ETriggerEvent::Completed, this, &ACPP_TopDownPlayerController::OnPlayerStopDodge);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACPP_TopDownPlayerController::OnInputStarted()
{
	StopMovement();
}

void ACPP_TopDownPlayerController::Move(const FInputActionValue& value)
{
	FVector2D movementVector = value.Get<FVector2D>();
	FVector inputVector = FVector(movementVector, 0);

	ABaseMagicCharacter* character = Cast<ABaseMagicCharacter>(GetPawn());
	if(character) character->moveRotation = inputVector.Rotation();

	GetPawn()->AddMovementInput(inputVector, speed, false);

	// Hearing event
	// ——— Report a hearing stimulus anytime we actually move ———
	if (!inputVector.IsNearlyZero())
	{
		APawn* Me = GetPawn();
		UWorld* World = GetWorld();
		if (World && Me)
		{
			const float Loudness = 1.0f;      // adjust how loud footsteps are
			const float HearingRange = 500.f; // match your AI’s HearingConfig

			// Report the noise  
			UAISense_Hearing::ReportNoiseEvent(
				World,
				Me->GetActorLocation(),                     // who made the noise
				Loudness,               // volume
				Me, // where
				HearingRange            // how far it carries
			);
		}
	}
}

void ACPP_TopDownPlayerController::SelectFirstSpell(const FInputActionValue& value)
{
	if (PlayerCharacter)
	{
		PlayerCharacter->SetBulletTypeByIndex(0);
	}
}

void ACPP_TopDownPlayerController::SelectSecondSpell(const FInputActionValue& value)
{
	if (PlayerCharacter)
	{
		// index 1 = your second bullet type
		PlayerCharacter->SetBulletTypeByIndex(1);
	}
}

void ACPP_TopDownPlayerController::FireBullet(const FInputActionValue& value)
{
	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());
	FVector Direction = FVector(value.Get<FVector2D>(), 0);

	if (BPlayerCharacter->CurrentBulletTypeIndex == 0) {
		if (!Direction.IsNearlyZero())
		{
			Direction.Normalize();

			// 3) Compute final velocity
			float ShootSpeed = BPlayerCharacter->CharacterStats.shootSpeed;
			FVector Velocity = Direction * ShootSpeed;

			// 4) Fire with that velocity
			BPlayerCharacter->ShootBullet(Velocity);
		}
	}

}


void ACPP_TopDownPlayerController::MeleeAttack(const FInputActionValue& value)
{
	if (PlayerCharacter) {
		PlayerCharacter->MeleeAttack();
	}
}

//void ACPP_TopDownPlayerController::Dodge(const FInputActionValue& value)
//{
//	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());
//	if (BPlayerCharacter) BPlayerCharacter->Dodge();
//}

void ACPP_TopDownPlayerController::OnPlayerStartShooting()
{
	ABaseMagicCharacter* character = Cast<ABaseMagicCharacter>(GetPawn());
	if (character) character->OnStartShooting();
}

void ACPP_TopDownPlayerController::OnPlayerStopShooting()
{
	ABaseMagicCharacter* character = Cast<ABaseMagicCharacter>(GetPawn());
	if (character) character->OnStopShooting();
}

void ACPP_TopDownPlayerController::OnPlayerStartArc(const FInputActionValue& value)
{
	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());

	// read the 2D keypad/stick direction
	FVector2D Input2D = value.Get<FVector2D>();
	FVector Direction = FVector(Input2D, 0.f);

	// update aim immediately (so charging will use correct shootRotation)
	if (!Direction.IsNearlyZero())
	{
		Direction.Normalize();
		BPlayerCharacter->SetAimFromDirection(Direction);
	}

	if (BPlayerCharacter->CurrentBulletTypeIndex == 1) BPlayerCharacter->OnStartArcCharge();
}

void ACPP_TopDownPlayerController::OnPlayerReleaseArc(const FInputActionValue& value)
{
	if (GEngine) {
		FString Msg = FString::Printf(TEXT("OnPlayerReleaseArc called"));
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
	}
	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());
	if (BPlayerCharacter->CurrentBulletTypeIndex == 1) BPlayerCharacter->OnReleaseArcThrow();

}

void ACPP_TopDownPlayerController::OnPlayerStartMelee()
{
	ABaseMagicCharacter* character = Cast<ABaseMagicCharacter>(GetPawn());
	if (character) character->OnStartMelee();
}

void ACPP_TopDownPlayerController::OnPlayerStopMelee()
{
	ABaseMagicCharacter* character = Cast<ABaseMagicCharacter>(GetPawn());
	if (character) character->OnStopMelee();
}

void ACPP_TopDownPlayerController::OnPlayerStartDodge()
{
	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());
	if (BPlayerCharacter) BPlayerCharacter->OnStartDodge();
}

void ACPP_TopDownPlayerController::OnPlayerStopDodge()
{
	ABasePlayerCharacter* BPlayerCharacter = Cast<ABasePlayerCharacter>(GetPawn());
	if (BPlayerCharacter) BPlayerCharacter->OnStopDodge();
}

