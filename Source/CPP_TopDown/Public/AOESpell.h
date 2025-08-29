// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseBullet.h"
#include "AOESpell.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API AAOESpell : public ABaseBullet
{
	GENERATED_BODY()
	
public:

	// Sets default values for this actor's properties
	AAOESpell();

    /** Radius of your AOE explosion */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grenade")
    float ExplosionRadius = 300.0f;

protected:

	virtual void BeginPlay() override;

	virtual void OnComponentHit(UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

    /** Called after fuse or on hit */
    void Explode();

	/** Damage at center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float ExplosionDamage = 50.0f;

	/** VFX for the explosion (Niagara or Cascade) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	UNiagaraSystem* ExplosionParticles;

	/** How strong the radial knockback is (horizontal) */
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float KnockbackStrength = 1200.0f;

	/** Upwards lift component added to the knockback (vertical) */
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float KnockbackZ = 350.0f;

	/** Radius used for knockback (defaults to ExplosionRadius) */
	UPROPERTY(EditDefaultsOnly, Category = "Explosion")
	float KnockbackRadius = 0.0f; // 0 = use ExplosionRadius

};
