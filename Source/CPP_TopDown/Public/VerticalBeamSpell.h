// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PooledActor.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "VerticalBeamSpell.generated.h"


UCLASS()
class CPP_TOPDOWN_API AVerticalBeamSpell : public APooledActor
{
	GENERATED_BODY()
	
public:
    AVerticalBeamSpell();

    /** Initialize the beam after spawning. Call this from the spawner (enemy). */
    void Initialize(AActor* TargetActor, float Duration = 2.0f, float BeamScale = 1.0f, float DamagePerSecond = 20.0f, float TickInterval = 0.2f);

protected:
    virtual void BeginPlay() override;

    // Niagara component created as a default subobject so it is visible in BP
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Beam", meta = (AllowPrivateAccess = "true"))
    UNiagaraComponent* NiagaraComp;

    // Assign this Niagara System in the BP (or leave empty and set at runtime)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam")
    UNiagaraSystem* BeamFX;

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* Collider;

    // Settings (can also be set via Initialize)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Beam", meta = (AllowPrivateAccess = "true"))
    float Lifetime;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Beam", meta = (AllowPrivateAccess = "true"))
    float Scale;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Beam", meta = (AllowPrivateAccess = "true"))
    float DPS;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Beam", meta = (AllowPrivateAccess = "true"))
    float DamageTickInterval;

    // Delay before enabling damage (seconds). Set to 1.0 for your warning phase.
    UPROPERTY(EditDefaultsOnly, Category = "Beam")
    float DamageStartDelay = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UAnimMontage* GetHitAnim_Montage;

    /** Stagger duration we want to enforce regardless of montage length */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger")
    float StaggerDurationOverride = 0.3f;

    /** Small buffer before re-applying stagger again (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger")
    float StaggerCooldownBuffer = 0.05f;

    /** Map of actor -> world time (seconds) when next stagger is allowed */
    TMap<TWeakObjectPtr<AActor>, float> NextAllowedStaggerTime;

    // pruning control
    UPROPERTY(EditAnywhere, Category = "Stagger")
    float PruneIntervalSeconds = 5.0f;

    float LastPruneTime = 0.0f;

    // function declaration
    void PruneStaggerCooldowns(float MaxAgeSeconds = 5.0f);

    // Called when the warning period ends — enables collider and starts damage ticking.
    UFUNCTION()
    void StartDamagePhase();

    // Overlapping actors collected (weak ptrs)
    TSet<TWeakObjectPtr<AActor>> OverlappingActors;

    // Timers
    FTimerHandle DamageStartTimerHandle;
    FTimerHandle DamageTickTimer;
    FTimerHandle LifeTimer;

    // Collision handlers
    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // periodic damage
    void ApplyDamageTick();

    // finish
    UFUNCTION()
    void EndBeam();

    // helper to configure collider extents
    void ConfigureCollider();

public:

	virtual void Tick(float DeltaTime) override;

};
