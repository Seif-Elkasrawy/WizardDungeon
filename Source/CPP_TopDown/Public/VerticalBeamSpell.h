// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "VerticalBeamSpell.generated.h"


UCLASS()
class CPP_TOPDOWN_API AVerticalBeamSpell : public AActor
{
	GENERATED_BODY()
	
public:
    AVerticalBeamSpell();

    /** Initialize the beam after spawning. Call this from the spawner (enemy). */
    void Initialize(AActor* TargetActor, bool bAttachToTarget = false, float Duration = 2.0f, float BeamScale = 1.0f, float DamagePerSecond = 20.0f, float TickInterval = 0.2f);

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

};
