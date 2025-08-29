// Fill out your copyright notice in the Description page of Project Settings.


#include "AOESpell.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

AAOESpell::AAOESpell()
{
	ExplosionRadius = 300.0f;
	ExplosionDamage = 50.0f;

	//// enable gravity for an arc
	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = 1.0f;
	}
	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
}

void AAOESpell::BeginPlay()
{
	Super::BeginPlay();
}

void AAOESpell::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Explode on impact
	Explode();
}

void AAOESpell::Explode()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float UseRadius = (KnockbackRadius > 0.0f) ? KnockbackRadius : ExplosionRadius;

	// spawn explosion FX
	if (ExplosionParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ExplosionParticles,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// Cache instigator controller (may be null)
	AController* InstigatorController = GetInstigatorController();

	// radial damage
	UGameplayStatics::ApplyRadialDamage(
		World,
		ExplosionDamage,
		GetActorLocation(),
		ExplosionRadius,
		DamageType,            // inherits from your bullet’s DamageType
		TArray<AActor*>(),     // ignore none
		this,                  // damage causer
		InstigatorController,
		true                   // full damage at inner radius
	);

    // ---- knockback ----
// Prepare overlap query to find pawns/actors inside radius
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // ignore the explosion actor itself

    // If you want to ignore the instigator, add:
    if (AActor* InstigatorPawn = GetInstigator())
    {
        QueryParams.AddIgnoredActor(InstigatorPawn);
    }

    FCollisionShape Sphere = FCollisionShape::MakeSphere(UseRadius);

    bool bHit = World->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,   // only query pawns — change if you want to catch physics objects too
        Sphere,
        QueryParams
    );

    if (bHit)
    {
        for (const FOverlapResult& R : Overlaps)
        {
            AActor* HitActor = R.GetActor();
            if (!IsValid(HitActor) || HitActor == this) continue;

            // compute direction from explosion center to actor
            FVector Dir = HitActor->GetActorLocation() - GetActorLocation();
            Dir.Z = 0.f; // horizontal dir
            Dir = Dir.GetSafeNormal();

            // final impulse / launch velocity
            FVector LaunchVel = Dir * KnockbackStrength + FVector(0.f, 0.f, KnockbackZ);

            // If it's a Character, use LaunchCharacter (plays nicely with CharacterMovement)
            if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
            {
                // LaunchCharacter adds to existing velocity; the two bools allow XY override and Z override
                HitChar->LaunchCharacter(LaunchVel, true, true);
            }
            else
            {
                // Try to apply to primitive component if it simulates physics
                UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(R.GetComponent());
                if (Prim && Prim->IsSimulatingPhysics())
                {
                    // Add an impulse at actor location. Multiply by Prim->GetMass() if you want consistent acceleration.
                    Prim->AddImpulse(LaunchVel * Prim->GetMass());
                }
            }
        }
    }

	// Destroy the spell actor
	Destroy();
}

