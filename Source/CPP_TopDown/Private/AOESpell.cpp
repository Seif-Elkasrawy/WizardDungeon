// Fill out your copyright notice in the Description page of Project Settings.


#include "AOESpell.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include <BaseMagicCharacter.h>
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
	PrimaryActorTick.bCanEverTick = false;
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

    // 1) Gather all overlapping pawns/actors BEFORE applying radial damage.
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // ignore the explosion actor itself

    if (AActor* InstPawn = GetInstigator())
    {
        QueryParams.AddIgnoredActor(InstPawn);
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

    // Build a list of unique actors to affect (avoid duplicates)
    TArray<TWeakObjectPtr<AActor>> AffectedActors;
    AffectedActors.Reserve(Overlaps.Num());

    if (bHit)
    {
        for (const FOverlapResult& R : Overlaps)
        {
            AActor* HitActor = R.GetActor();
            if (!IsValid(HitActor) || HitActor == this) continue;
            if (!AffectedActors.Contains(TWeakObjectPtr<AActor>(HitActor)))
            {
                AffectedActors.Add(TWeakObjectPtr<AActor>(HitActor));
            }
        }
    }

    // 2) Spawn explosion FX (visual)
    if (ExplosionParticles)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            ExplosionParticles,
            GetActorLocation(),
            GetActorRotation()
        );
    }

    // 3) Apply radial damage once (server authoritative)
    AController* InstigatorController = GetInstigatorController();
    UGameplayStatics::ApplyRadialDamage(
        World,
        ExplosionDamage,
        GetActorLocation(),
        ExplosionRadius,
        DamageType,
        TArray<AActor*>(), // ignore none (we already excluded instigator via QueryParams)
        this,
        InstigatorController,
        true
    );

    // 4) For each previously-queried actor: if still valid, try to play montage and stagger them, then knockback.
    const float Now = World->GetTimeSeconds();

    for (TWeakObjectPtr<AActor> WeakHit : AffectedActors)
    {
        AActor* HitActor = WeakHit.Get();
        if (!IsValid(HitActor)) continue; // may have been destroyed by damage

        // --- Stagger: check cooldown per-actor to avoid re-staggering same target too quickly ---
        float* NextAllowedPtr = NextAllowedStaggerTime.Find(WeakHit);
        if (NextAllowedPtr && Now < *NextAllowedPtr)
        {
            // still cooling down
        }
        else
        {
            // Try to play the montage and apply stagger
            if (GetHitAnim_Montage)
            {
                USkeletalMeshComponent* MeshComp = nullptr;
                if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
                {
                    MeshComp = HitChar->GetMesh();
                }
                else
                {
                    MeshComp = HitActor->FindComponentByClass<USkeletalMeshComponent>();
                }

                if (MeshComp)
                {
                    if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
                    {
                        // Optionally avoid restarting montage if it's already playing
                        if (!AnimInst->Montage_IsPlaying(GetHitAnim_Montage))
                        {
                            AnimInst->Montage_Play(GetHitAnim_Montage, 1.0f);
                        }

                        // Apply a short fixed stagger rather than using full montage duration
                        const float StaggerDur = (StaggerDurationOverride > 0.f) ? StaggerDurationOverride : AnimInst->Montage_Play(GetHitAnim_Montage, 1.0f);

                        if (ABaseMagicCharacter* MagicChar = Cast<ABaseMagicCharacter>(HitActor))
                        {
                            MagicChar->EnterStagger(StaggerDur);
                        }

                        // set cooldown so we don't re-stagger immediately
                        NextAllowedStaggerTime.Add(WeakHit, Now + StaggerDur + StaggerCooldownBuffer);
                    }
                }
                else
                {
                    // No mesh - still try to stagger if it's our BaseMagicCharacter
                    if (ABaseMagicCharacter* MagicChar = Cast<ABaseMagicCharacter>(HitActor))
                    {
                        MagicChar->EnterStagger(StaggerDurationOverride);
                        NextAllowedStaggerTime.Add(WeakHit, Now + StaggerDurationOverride + StaggerCooldownBuffer);
                    }
                }
            } // end if GetHitAnim_Montage
        } // end cooldown check

        // ---- Knockback (apply after the stagger attempt) ----
        // compute direction from explosion center to actor
        FVector Dir = HitActor->GetActorLocation() - GetActorLocation();
        Dir.Z = 0.f; // horizontal dir
        Dir = Dir.GetSafeNormal();

        FVector LaunchVel = Dir * KnockbackStrength + FVector(0.f, 0.f, KnockbackZ);

        if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
        {
            HitChar->LaunchCharacter(LaunchVel, true, true);
        }
        else
        {
            // Try to apply to primitive component if it simulates physics
            // prefer the overlap's component if available (we don't have it here), fallback to root primitive
            UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
            if (Prim && Prim->IsSimulatingPhysics())
            {
                Prim->AddImpulse(LaunchVel * Prim->GetMass());
            }
        }
    } // end for each affected actor

    // 5) Return spell actor to pool
    ReturnToPool();
}


