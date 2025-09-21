// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "BaseMagicCharacter.h"
#include "Player_HUD.h"
#include "HPWidgetBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "BasePlayerCharacter.generated.h"



/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API ABasePlayerCharacter : public ABaseMagicCharacter
{
	GENERATED_BODY()

public:

	ABasePlayerCharacter();

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	virtual void BeginPlay() override;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY()
	UPlayer_HUD* PlayerHUDWidget = nullptr;

	// store pointer
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UPlayer_HUD> Player_HUDClass;

	virtual float TakeDamage(
		float DamageCount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	)override;

	// == Charging for arc throw ==
	bool bChargingArc = false;
	float CurrentChargeTime = 0.f;

	/** How long you can charge, in seconds */
	UPROPERTY(EditAnywhere, Category = "Combat|Arc")
	float MaxChargeTime = 3.0f;

	/** The maximum horizontal distance the grenade can travel when fully charged */
	UPROPERTY(EditAnywhere, Category = "Combat|Arc")
	float MaxArcRange = 1200.f;

	/** The gravity scale to apply to the grenade (default PhysX=-980 cm/s²) */
	UPROPERTY(EditAnywhere, Category = "Combat|Arc")
	float ProjectileGravity = 980.f;

	// NEW: store the latest computed velocity for use on release / preview
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArcThrow")
	FVector PendingArcVelocity = FVector::ZeroVector;

	// dissolve
	UPROPERTY(EditAnywhere, Category = "Teleport|Dissolve")
	FName DissolveParameterName = "DissolveAmount";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dissolve")
	float DissolveValueMin = -0.032f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dissolve")
	float DissolveValueMax = 0.67f;

	UPROPERTY(EditAnywhere, Category = "Teleport|Dissolve")
	float DissolveDuration = 0.22f;

	UPROPERTY(EditAnywhere, Category = "Teleport|Dissolve")
	float ResolveDuration = 0.2f;

	// Runtime
	TArray<UMaterialInstanceDynamic*> DynamicMats;
	TArray<UMaterialInterface*> OriginalMats;
	FTimerHandle TimerHandle_DissolveTick;
	float DissolveElapsed = 0.f;
	bool bDissolvingOut = false; // true = dissolving (hide); false = resolving (show)

	/// <summary>
	/// Teleport / Dodge Settings
	/// </summary>
	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeDistance = 800.0f;                     // how far to teleport (cm)

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeCooldown = 1.0f;                       // seconds between dodges

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeInvulnerabilityTime = 0.2f;            // brief invuln during/after dodge (optional)

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeSweepRadius = 34.0f;                   // capsule radius for sweep (cm)

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeSweepHalfHeight = 88.0f;               // capsule half-height for sweep (cm)

	// members
	// Niagara component (optional: designer can assign the system asset in BP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Dissolve", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* DissolveNiagaraComp = nullptr;

	// if you prefer to assign a Niagara asset instead of baking it to the comp:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Dissolve")
	UNiagaraSystem* DissolveNiagaraSystem = nullptr;

	// teleport bookkeeping
	FVector PendingTeleportLocation;
	bool bHasPendingTeleport = false;

	// callback from Niagara finished (if you use it)
	UFUNCTION()
	void OnDissolveNiagaraFinished(UNiagaraComponent* FinishedComponent);

	// helper that actually performs teleport when we are fully dissolved
	void PerformTeleportNow();

	// runtime state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	bool bCanDodge = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	bool bIsDodgeInvulnerable = false;

	FTimerHandle TimerHandle_DodgeCooldown;
	FTimerHandle TimerHandle_EndInvuln;

	void InitDissolveMIDs();
	void StartDissolveOut();   // begin dissolve (0 -> 1)
	void UpdateDissolveTick(); // called by timer

	// indicator blueprint class to set in editor
	UPROPERTY(EditAnywhere, Category = "ArcPreview")
	TSubclassOf<AActor> LandingIndicatorClass;

	UPROPERTY(EditAnywhere, Category = "ArcPreview")
	AActor* LandingIndicatorActor;

	float IndicatorMeshDefaultRadius = 50.0f; // cm

	/** The class of your grenade projectile */
	UPROPERTY(EditAnywhere, Category = "Combat|Arc")
	TSubclassOf<class AAOESpell> AOESpellClass;

	// Functions
	UFUNCTION()
	void ResetDodge();

	UFUNCTION()
	void EndDodgeInvuln();

public:

	void OnStartArcCharge();
	FVector ArcCharging();
	void OnReleaseArcThrow();
	void UpdateArcPreview();
	void UpdateLandingIndicator(const FVector& Location, const FVector& Normal = FVector::ZeroVector, float ExplosionRadius = 0);
	void HideLandingIndicator();
	void SetAimFromDirection(const FVector& Direction3D);

	UFUNCTION()
	void Dodge();

	UFUNCTION()
	void OnStartDodge();

	UFUNCTION()
	void OnStopDodge();
	
};
