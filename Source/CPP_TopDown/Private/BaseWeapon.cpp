// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon Mesh"));

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseWeapon::WeaponShoot()
{

}

void ABaseWeapon::SetPlayerPointer(ACharacter* PlayerPtr)
{
	Player = PlayerPtr;
}


// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

