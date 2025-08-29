// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseOpenDoor.h"


// Sets default values
ABaseOpenDoor::ABaseOpenDoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootScene;

    // Create left and right door components
    LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
    LeftDoor->SetupAttachment(RootScene);
    RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
    RightDoor->SetupAttachment(RootScene);
}

// Called when the game starts or when spawned
void ABaseOpenDoor::BeginPlay()
{
	Super::BeginPlay();
	
    // Store initial rotations
    InitialLeftRot = LeftDoor->GetRelativeRotation();
    InitialRightRot = RightDoor->GetRelativeRotation();

    // Calculate target rotations
    TargetLeftRot = InitialLeftRot + FRotator(0.f, -OpenAngle, 0.f);
    TargetRightRot = InitialRightRot + FRotator(0.f, OpenAngle, 0.f);

    RemainingEnemies = EnemiesToWatch.Num();

    if (RemainingEnemies == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] has no enemies assigned; opening immediately."), *GetName());
        OpenDoor();
        return;
    }

    for (ABaseMagicCharacter* Enemy : EnemiesToWatch)
    {
        if (Enemy) {
            // Bind to the enemy's death delegate
            Enemy->OnDeath.AddDynamic(this, &ABaseOpenDoor::HandleEnemyDeath);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("[%s] EnemiesToWatch contains a null entry!"), *GetName());
            // You could decide to decrement RemainingEnemies here, or fix your Blueprint.
        }
    }
}

void ABaseOpenDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FRotator CurrentRotation = GetActorRotation();
    float CurrentYaw = CurrentRotation.Yaw;

    if (bOpen)
    {
        // Interpolate left
        FRotator CurrentLeft = LeftDoor->GetRelativeRotation();
        FRotator NewLeft = FMath::RInterpTo(CurrentLeft, TargetLeftRot, DeltaTime, DoorOpenSpeed);
        LeftDoor->SetRelativeRotation(NewLeft);

        // Interpolate right
        FRotator CurrentRight = RightDoor->GetRelativeRotation();
        FRotator NewRight = FMath::RInterpTo(CurrentRight, TargetRightRot, DeltaTime, DoorOpenSpeed);
        RightDoor->SetRelativeRotation(NewRight);

        // Check if done
        if (NewLeft.Equals(TargetLeftRot, 0.1f) && NewRight.Equals(TargetRightRot, 0.1f))
        {
            bOpen = false;
        }
    }
    else if (bClose)
    {
        // Interpolate back to initial
        FRotator CurrentLeft = LeftDoor->GetRelativeRotation();
        FRotator NewLeft = FMath::RInterpTo(CurrentLeft, InitialLeftRot, DeltaTime, DoorOpenSpeed);
        LeftDoor->SetRelativeRotation(NewLeft);

        FRotator CurrentRight = RightDoor->GetRelativeRotation();
        FRotator NewRight = FMath::RInterpTo(CurrentRight, InitialRightRot, DeltaTime, DoorOpenSpeed);
        RightDoor->SetRelativeRotation(NewRight);

        if (NewLeft.Equals(InitialLeftRot, 0.1f) && NewRight.Equals(InitialRightRot, 0.1f))
        {
            bClose = false;
        }
    }
}

void ABaseOpenDoor::OpenDoor()
{
    bClose = false;
    bOpen = true;
}

void ABaseOpenDoor::CloseDoor()
{
    bOpen = false;
    bClose = true;
}

void ABaseOpenDoor::HandleEnemyDeath(ABaseMagicCharacter* DeadEnemy)
{
    // Safety: make sure we don’t go negative
    RemainingEnemies = FMath::Max(0, RemainingEnemies - 1);
    UE_LOG(LogTemp, Log, TEXT("[%s] Enemy died. %d remaining."), *GetName(), RemainingEnemies);

    if (RemainingEnemies == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] All enemies dead—opening door."), *GetName());
        OpenDoor();
    }
}

