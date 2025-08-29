// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EEnemyStates.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EEnemyStates: uint8
{
	Passive UMETA(DisplayName = "Passive"),		// Not aware of player, patrolling
	Investigating UMETA(DisplayName = "Investigating"),	// Investigating sound
	Alerted UMETA(DisplayName = "Alerted"),		// Aware of player, but not attacking
	Attacking UMETA(DisplayName = "Attacking"),		// Actively attacking the player
	Stunned UMETA(DisplayName = "Stunned"),		// Temporarily incapacitated
	Dead	UMETA(DisplayName = "Dead")		// Enemy is dead
};
