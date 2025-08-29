// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "BaseMagicCharacter.h"
#include "Player_HUD.h"
#include "HPWidgetBase.h"
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

	UPROPERTY(VisibleAnywhere, Category = "ArcPreview")
	USplineComponent* TrajectorySpline;

	UPROPERTY(EditAnywhere, Category = "ArcPreview")
	UStaticMesh* SplineMesh;

	// optionally keep spline mesh components if you want thick mesh segments
	UPROPERTY()
	TArray<class USplineMeshComponent*> TrajectorySplineMeshes;


	// indicator blueprint class to set in editor
	UPROPERTY(EditAnywhere, Category = "ArcPreview")
	TSubclassOf<AActor> LandingIndicatorClass;

	UPROPERTY(EditAnywhere, Category = "ArcPreview")
	AActor* LandingIndicatorActor;

	float IndicatorMeshDefaultRadius = 50.0f; // cm

	/** The class of your grenade projectile */
	UPROPERTY(EditAnywhere, Category = "Combat|Arc")
	TSubclassOf<class AAOESpell> AOESpellClass;

public:

	void OnStartArcCharge();
	FVector ArcCharging();
	void OnReleaseArcThrow();
	void UpdateArcPreview();
	void UpdateLandingIndicator(const FVector& Location, const FVector& Normal = FVector::ZeroVector, float ExplosionRadius = 0);
	void HideLandingIndicator();
	void SetAimFromDirection(const FVector& Direction3D);
	
};
