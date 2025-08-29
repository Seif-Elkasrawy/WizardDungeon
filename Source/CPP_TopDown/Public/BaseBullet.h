// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseBullet.generated.h"

UCLASS()
class CPP_TOPDOWN_API ABaseBullet : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseBullet();

	UPROPERTY(EditDefaultsOnly)
	class UProjectileMovementComponent* ProjectileMovement;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
	class UNiagaraComponent* BulletFX;

	UPROPERTY(EditDefaultsOnly)
	class USphereComponent* collisionSphere;

	UFUNCTION()
	virtual void OnComponentHit(UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);


	virtual void BulletHit(AActor* OtherActor, const FHitResult& Hit);

	UPROPERTY(EditDefaultsOnly)
	class UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditDefaultsOnly)
	float baseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageType> DamageType;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly)
	float speed;

};
