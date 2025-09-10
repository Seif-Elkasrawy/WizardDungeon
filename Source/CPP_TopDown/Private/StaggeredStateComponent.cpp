// Fill out your copyright notice in the Description page of Project Settings.


#include "StaggeredStateComponent.h"
#include "BaseMagicCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UStaggeredStateComponent::UStaggeredStateComponent()
{

	PrimaryComponentTick.bCanEverTick = true;
	bIsStaggered = false;
	OwnerCharacter = nullptr;
	bDisabledInput = false;
	bDisabledMovement = false;

	// ...
}


// Called when the game starts
void UStaggeredStateComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABaseMagicCharacter>(GetOwner());
	// ...
	
}

void UStaggeredStateComponent::EnterStagger(float Duration)
{
	if (bIsStaggered)
	{
		// Already staggered, reset timer if applicable
		if (Duration > 0.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(StaggerTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(StaggerTimerHandle, this, &UStaggeredStateComponent::ExitStagger, Duration, false);
		}
		return;
	}

	bIsStaggered = true;
	ApplyBlock();

	if (Duration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(StaggerTimerHandle, this, &UStaggeredStateComponent::ExitStagger, Duration, false);
	}
}

void UStaggeredStateComponent::ExitStagger()
{
	if (!bIsStaggered) return;

	bIsStaggered = false;
	GetWorld()->GetTimerManager().ClearTimer(StaggerTimerHandle);
	RemoveBlock();
}

void UStaggeredStateComponent::ApplyBlock()
{
	if (!IsValid(OwnerCharacter)) return;

	// Stop and disable movement
	if (UCharacterMovementComponent* MoveComponent = OwnerCharacter->GetCharacterMovement()) {
		MoveComponent->StopMovementImmediately();
		MoveComponent->DisableMovement();
		bDisabledMovement = true;
	}
	else {
		bDisabledMovement = false;
	}

	// Disable input if controlled by a player
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController())) {
		PC->SetIgnoreMoveInput(true);
		bDisabledInput = true;
	}
	else {
		bDisabledInput = false;
	}

	// Block shooting/melee and other action flags (toggle variables on the character)
	// These members exist in your ABaseMagicCharacter: canFire, bCanMelee, isShooting, isMeleeAttacking, etc.
	// We'll call helper functions on character to keep encapsulation.
	OwnerCharacter->BlockAllActions();
}

void UStaggeredStateComponent::RemoveBlock()
{
	if (!IsValid(OwnerCharacter)) return;

	// Re-enable movement if we disabled it
	if (bDisabledMovement)
	{
		if (UCharacterMovementComponent* MoveComponent = OwnerCharacter->GetCharacterMovement()) {
			MoveComponent->SetMovementMode(MOVE_Walking);
		}
		bDisabledMovement = false;
	}
	// Re-enable input if we disabled it
	if (bDisabledInput)
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController())) {
			PC->SetIgnoreMoveInput(false);
		}
		bDisabledInput = false;
	}

	// Unblock shooting/melee and other action flags
	OwnerCharacter->UnblockAllActions();
}


// Called every frame
void UStaggeredStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

