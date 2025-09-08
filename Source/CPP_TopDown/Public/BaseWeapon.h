// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.generated.h"

class UPrimitiveComponent;
class UBoxComponent;

UCLASS()
class CPP_TOPDOWN_API ABaseWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	// Called by owner to enable overlap for the swing window
	UFUNCTION()
	void StartMeleeWindow(float WindowSeconds = 0.2f);

	UFUNCTION()
	void StopMeleeWindow();

	void SetPlayerPointer(ACharacter* PlayerPtr);

	// damage to apply (can be set by owner)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeDamage = 25.f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* WeaponMesh;

	// this is the collider that hits actors (attach to mesh/socket)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* HitCollider;

	// remember who we hit this window to avoid multi-hits
	TSet<TWeakObjectPtr<AActor>> HitActors;

	UFUNCTION(BlueprintCallable)
	void WeaponShoot();

	ACharacter* Player;

	FTimerHandle TimerHandle_StopWindow;

	UFUNCTION()
	void OnHitOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
