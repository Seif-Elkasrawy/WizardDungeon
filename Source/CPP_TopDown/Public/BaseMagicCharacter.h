// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" 
#include "BaseWeapon.h"
#include "StaggeredStateComponent.h"
#include "AC_ObjectPool.h"
#include "BaseMagicCharacter.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBulletFiredDelegate, AActor*, FiredActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeathDelegate, ABaseMagicCharacter*, DeadEnemy);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeStart, bool, bIsAttacking);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeFinished, bool, bIsAttacking);

USTRUCT(BlueprintType)
struct FCharacterStats
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float MaxHP;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float HP;

	/** Cooldown between melee attacks */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Melee")
	float MeleeCooldown;

	/** Effective reach of the melee attack */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Melee")
	float MeleeRange;

	/** Damage dealt per melee hit */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Melee")
	float MeleeDamage;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Ranged")
	float shootSpeed;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Ranged")
	float fireRate;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats|Movement")
	float movementSpeed;
};

UCLASS()
class CPP_TOPDOWN_API ABaseMagicCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseMagicCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaggeredStateComponent* StaggeredState;

	//Called     // Called by StaggerStateComponent to block/unblock actions on this character
	void BlockAllActions();
	void UnblockAllActions();

	// Broadcast when firing a bullet
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBulletFiredDelegate firedBullet;

	// Broadcast when this character dies
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDeathDelegate OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMeleeStart OnMeleeStart;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMeleeFinished OnMeleeFinished;

	// Forward the request to stagger component (useful for external callers like spells)
	UFUNCTION(BlueprintCallable, Category = "Stagger")
	void EnterStagger(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Stagger")
	void ExitStagger();

	UFUNCTION(BlueprintCallable, Category = "Stagger")
	bool IsStaggered() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UChildActorComponent* Weapon;

	ABaseWeapon* CachedWeapon;

	UPROPERTY(EditAnywhere, Category = "Pooling")
	UAC_ObjectPool* BaseSpellPool;

	/** All the bullet types this character can fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<class ABaseBullet>> AvailableBulletTypes;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ABaseBullet> BulletToSpawn;

	bool canFire = true;

	UPROPERTY(BlueprintReadOnly)
	bool bCanMelee = true;

	UPROPERTY(BlueprintReadOnly)
	bool isShooting;

	UPROPERTY(BlueprintReadOnly)
	bool isMeleeAttacking;

	bool bIsDead = false;

	// Visual hit flash settings (tweak in editor)
	UPROPERTY(EditDefaultsOnly, Category = "Damage|Visual")
	FLinearColor DamageTint = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, Category = "Damage|Visual")
	float DamageTintDuration = 0.12f;

	// Parameter names your material must expose
	UPROPERTY(EditDefaultsOnly, Category = "Damage|Visual")
	FName TintColorParamName = TEXT("HitFlashColor");

	UPROPERTY(EditDefaultsOnly, Category = "Damage|Visual")
	FName TintAmountParamName = TEXT("HitFlashAmount");

	// Runtime
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	FTimerHandle TimerHandle_HitFlash;

	// Helpers
	void StartHitFlash();
	void StopHitFlash();

	// Optional: function to ensure dynamic mats exist (call from BeginPlay)
	void EnsureDynamicMaterials();

	UFUNCTION()
	void SetCanFire(bool value);

	// Timer handle for resetting melee
	FTimerHandle MeleeTimerHandle;

	// Resets the ability to melee
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetMelee();

	UPROPERTY(EditAnywhere, Category = "Combat|Melee")
	float MeleeHitWindow = 0.1f;

	UFUNCTION(BlueprintPure)
	FVector CalculateMovementBlending();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Stats")
	FCharacterStats CharacterStats;

	virtual AActor* ShootBullet(const FVector& Velocity);

	// Performs the melee sweep and damage
	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void MeleeAttack();

	/** Switch directly to a specific bullet type slot */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetBulletTypeByIndex(int32 NewIndex);

	/** Which entry in AvailableBulletTypes we're currently using */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentBulletTypeIndex = 0;

	/** Switch to the next bullet type in the array */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ChangeBulletType();

	virtual float TakeDamage(
		float DamageCount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	)override;

	void OnStartShooting();
	void OnStopShooting();

	void OnStartMelee();
	void OnStopMelee();

	FRotator shootRotation;
	FRotator moveRotation;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* SpawnLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	FRotator DesiredAimRotation = FRotator::ZeroRotator;
};
