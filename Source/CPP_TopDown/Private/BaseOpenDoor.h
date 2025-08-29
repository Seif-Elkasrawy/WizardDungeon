// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemyCharacter.h"
#include "BaseOpenDoor.generated.h"


UCLASS()
class ABaseOpenDoor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseOpenDoor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    /** All of the enemies this door is watching */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Enemies")
    TArray<ABaseMagicCharacter*> EnemiesToWatch;

public:	

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Call this function to trigger door opening
	void OpenDoor();
	// Call this function to trigger door closing
	void CloseDoor();

private:
    // A neutral root so neither door drives the other's transform
    UPROPERTY(VisibleAnywhere)
    USceneComponent* RootScene;
    // Left and right door meshes
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LeftDoor;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* RightDoor;

    // Angle to open relative to initial rotation
    UPROPERTY(EditAnywhere, Category = "DoorConfig")
    float OpenAngle = 70.f;

    UPROPERTY(EditAnywhere, Category = "DoorConfig")
    float DoorOpenSpeed = 2.f;

    // Initial rotations
    FRotator InitialLeftRot;
    FRotator InitialRightRot;

    // Target rotations
    FRotator TargetLeftRot;
    FRotator TargetRightRot;

    // Whether door is currently opening
    bool bOpen = false;

    // Whether door is currently closing
    bool bClose = false;

    /** How many are still alive? */
    int32 RemainingEnemies;

    // Function to handle enemy death
    UFUNCTION()
    void HandleEnemyDeath(ABaseMagicCharacter* DeadEnemy);
};
