// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseMagicCharacter.h"
#include "BaseWeapon.h"
#include "BaseBullet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISense_Damage.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ABaseMagicCharacter::ABaseMagicCharacter()
{
	Weapon = CreateDefaultSubobject<UChildActorComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), TEXT("WeaponSocket")); // To parent Weapon to Mesh

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Bullet Spawn Point"));
	SpawnLocation->SetupAttachment(Weapon, TEXT("SpellSocket"));

    CharacterStats.MaxHP = 50.f;
    CharacterStats.HP = CharacterStats.MaxHP;
	CharacterStats.movementSpeed = 800;
    CharacterStats.MeleeCooldown = 1.0f;
    CharacterStats.MeleeDamage = 25.0f;
    CharacterStats.MeleeRange = 200.0f;
	CharacterStats.shootSpeed = 1200;
	CharacterStats.fireRate = 0.75f;
}

// Called when the game starts or when spawned
void ABaseMagicCharacter::BeginPlay()
{
	Super::BeginPlay();

    // make sure BulletToSpawn starts as the first type:
    if (AvailableBulletTypes.Num() > 0)
    {
        BulletToSpawn = AvailableBulletTypes[0];
        CurrentBulletTypeIndex = 0;
    }
	
	ABaseWeapon* weaponPtr = Cast<ABaseWeapon>(Weapon->GetChildActor()); // if this fails, it will be a nullptr so have to do a null check

	if (weaponPtr) {
		weaponPtr->SetPlayerPointer(this);
        weaponPtr->MeleeDamage = CharacterStats.MeleeDamage; // sync if you want
        CachedWeapon = weaponPtr; // store a pointer for quick access
	}

	GetCharacterMovement()->MaxWalkSpeed = CharacterStats.movementSpeed;
}

float ABaseMagicCharacter::TakeDamage(float DamageCount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Defensive early-outs
    if (DamageCount <= 0.f) return 0.f;
    if (bIsDead) return 0.f; // you should add bIsDead bool in header and initialize it false

	// Apply damage to the character's health
	CharacterStats.HP -= DamageCount;

    StartHitFlash();

    // If died, mark dead but DON'T Destroy() right away (defer destruction)
	if (CharacterStats.HP <= 0) {
        bIsDead = true;

		// Broadcast death event
		OnDeath.Broadcast(this);

        // Disable collisions, input, and tick as appropriate to avoid further interactions
        if (UCapsuleComponent* Cap = FindComponentByClass<UCapsuleComponent>())
        {
            Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        // Optionally disable movement/input here
        APawn* Pawn = Cast<APawn>(this);
        if (Pawn)
        {
            Pawn->DisableInput(nullptr);
        }

        // Defer actual destruction to avoid reentrancy problems (caller may still be iterating/expecting us)
        // Small delay to let the caller finish its stack (tweak delay as needed)
        SetLifeSpan(1.0f);
	}

    // --- fire off a Damage sense event ---
    if (UWorld* World = GetWorld())
    {
        AActor* InstigatorActor = nullptr;
        FVector InstigatorLocation = GetActorLocation(); // fallback to ourselves

        if (EventInstigator)
        {
            // EventInstigator might be valid but its pawn might be null—guard it
            if (APawn* InstPawn = EventInstigator->GetPawn())
            {
                InstigatorActor = InstPawn;
                InstigatorLocation = InstPawn->GetActorLocation();
            }
            else
            {
                // fallback: use controller's location if any (rare)
                if (AActor* ControllerActorOwner = EventInstigator->GetOwner())
                {
                    InstigatorLocation = ControllerActorOwner->GetActorLocation();
                }
            }
        }
        else if (IsValid(DamageCauser))
        {
            // If we have a damage-causer actor (e.g. bullet/AOE), use its location
            InstigatorLocation = DamageCauser->GetActorLocation();
            InstigatorActor = DamageCauser;
        }

        UAISense_Damage::ReportDamageEvent(
            World,
            this,              // the damaged actor (Stimulus recipient)
            InstigatorActor,      // the instigator / damage causer
            DamageCount,       // how much
            InstigatorLocation,// where it happened
            GetActorLocation()// direction (optional)
        );

        //if (GEngine)
        //{
        //    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
        //        FString::Printf(TEXT("[DamageSense] Reported damage to AI from %s (instigator: %s)"),
        //            *GetNameSafe(this),
        //            *GetNameSafe(InstigatorActor)));
        //}
    }

	return DamageCount;
}

void ABaseMagicCharacter::ChangeBulletType()
{
    if (AvailableBulletTypes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ChangeBulletType: no bullet types set"), *GetName());
        return;
    }

    // advance and wrap
	CurrentBulletTypeIndex = (CurrentBulletTypeIndex + 1) % AvailableBulletTypes.Num();
    BulletToSpawn = AvailableBulletTypes[CurrentBulletTypeIndex];

    // Feedback for you &/or UI
    if (GEngine)
    {
        FString Msg = FString::Printf(
            TEXT("[%s] Switched to bullet #%d: %s"),
            *GetName(),
            CurrentBulletTypeIndex,
            *BulletToSpawn->GetName()
        );
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
    }
}

// Called every frame
void ABaseMagicCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

AActor* ABaseMagicCharacter::ShootBullet(const FVector& Velocity)
{
    // 1) Compute the full rotation
    FRotator FullRot = Velocity.Rotation();

    // 2) Strip out pitch & roll
    FRotator YawOnlyRot(0.f, FullRot.Yaw, 0.f);
    // only affect playercontroller
    if (GetController()->IsPlayerController())
    {
        shootRotation = YawOnlyRot;
        SetActorRotation(shootRotation);
	}


    if (!canFire)
    {
        return nullptr;
    }
    canFire = false;

    // 1) Sanity–check your spawn data:
    if (!BulletToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ShootBullet: BulletToSpawn is null!"), *GetName());
        return nullptr;
    }
    if (!SpawnLocation)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ShootBullet: SpawnLocation is null!"), *GetName());
        return nullptr;
    }
    if (Velocity.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ShootBullet: zero‐length direction!"), *GetName());
        return nullptr;
    }

    // 2) Fire‐rate timer
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        FTimerDelegate::CreateUObject(this, &ABaseMagicCharacter::SetCanFire, true),
        CharacterStats.fireRate,
        false);

    // 3) Spawn the bullet
    FActorSpawnParameters SpawnParams;
    SpawnParams.Instigator = this;
    SpawnParams.Owner = this;     // also set owner, so GetInstigator() works

    FRotator GeneralShootRotation = YawOnlyRot;

    auto* Bullet = GetWorld()->SpawnActor<ABaseBullet>(
        BulletToSpawn,
        SpawnLocation->GetComponentLocation(),
        GeneralShootRotation,
        SpawnParams);

 //   if (GEngine)
 //   {
 //       FString Msg = FString::Printf(
 //           TEXT("[%s] ShootBullet: Spawned bullet %s at %s"),
 //           *GetName(),
 //           *BulletToSpawn->GetName(),
 //           *SpawnLocation->GetComponentLocation().ToString()
 //       );
 //       GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, Msg);
	//}

    if (!Bullet)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ShootBullet: SpawnActor returned null!"), *GetName());
        return nullptr;
    }

    // 4) Configure its movement
    if (Bullet->ProjectileMovement)
    {
        Bullet->ProjectileMovement->InitialSpeed = CharacterStats.shootSpeed;
        Bullet->ProjectileMovement->MaxSpeed = CharacterStats.shootSpeed;
        Bullet->ProjectileMovement->Velocity = Velocity;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ShootBullet: spawned bullet has no ProjectileMovement!"), *GetName());
    }

    // 5) Broadcast and return
    firedBullet.Broadcast(this);
    return Bullet;
}

void ABaseMagicCharacter::StartHitFlash()
{
    // Ensure we have dynamic materials
    if (DynamicMaterials.Num() == 0)
    {
        EnsureDynamicMaterials();
        if (DynamicMaterials.Num() == 0) return; // nothing to do
    }

    // Clear any pending timer so repeated hits refresh the flash
    GetWorldTimerManager().ClearTimer(TimerHandle_HitFlash);

    for (UMaterialInstanceDynamic* Dyn : DynamicMaterials)
    {
        if (!IsValid(Dyn)) continue;
        // set color and amount
        Dyn->SetVectorParameterValue(TintColorParamName, DamageTint);
        Dyn->SetScalarParameterValue(TintAmountParamName, 1.0f);
    }

    // Schedule stop
    GetWorldTimerManager().SetTimer(TimerHandle_HitFlash, this, &ABaseMagicCharacter::StopHitFlash, DamageTintDuration, false);
}

void ABaseMagicCharacter::StopHitFlash()
{
    // Cancel if no dynamics
    if (DynamicMaterials.Num() == 0) return;

    for (UMaterialInstanceDynamic* Dyn : DynamicMaterials)
    {
        if (!IsValid(Dyn)) continue;
        Dyn->SetScalarParameterValue(TintAmountParamName, 0.0f);
    }
}

void ABaseMagicCharacter::EnsureDynamicMaterials()
{
    DynamicMaterials.Empty();

    // Try skeletal mesh first
    if (USkeletalMeshComponent* Skel = FindComponentByClass<USkeletalMeshComponent>())
    {
        int32 Num = Skel->GetNumMaterials();
        for (int32 i = 0; i < Num; ++i)
        {
            UMaterialInterface* Mat = Skel->GetMaterial(i);
            UMaterialInstanceDynamic* Dyn = Skel->CreateAndSetMaterialInstanceDynamic(i);
            if (Dyn) DynamicMaterials.Add(Dyn);
        }
    }
    else
    {
        // Fallback: any mesh components
        TArray<UMeshComponent*> MeshComps;
        GetComponents<UMeshComponent>(MeshComps);
        for (UMeshComponent* Char_Mesh : MeshComps)
        {
            int32 Num = Char_Mesh->GetNumMaterials();
            for (int32 i = 0; i < Num; ++i)
            {
                // Create and replace material with dynamic instance
                UMaterialInstanceDynamic* Dyn = Char_Mesh->CreateAndSetMaterialInstanceDynamic(i);
                if (Dyn) DynamicMaterials.Add(Dyn);
            }
        }
    }
}

void ABaseMagicCharacter::SetCanFire(bool value)
{
	canFire = true;
}

void ABaseMagicCharacter::MeleeAttack()
{
    if (!bCanMelee)
        return;

    bCanMelee = false;
    isMeleeAttacking = true;
    // tell the AnimBP we just started an attack
    OnMeleeStart.Broadcast(true);

    // If we have a weapon that can do collisions, use it
    if (CachedWeapon)
    {
        // start a short hit window; tune 0.15..0.4 s depending on anim
        CachedWeapon->StartMeleeWindow(MeleeHitWindow);
    }
    //else {
    //    // Fallback to SweepMultiByChannelApproach
    //    // Calculate start/end points of your sweep
    //    FVector Start = GetActorLocation();
    //    FVector Forward = GetActorForwardVector();
    //    FVector End = Start + Forward * CharacterStats.MeleeRange;

    //    // Do a sphere or capsule sweep
    //    TArray<FHitResult> Hits;
    //    FCollisionShape Shape = FCollisionShape::MakeSphere(CharacterStats.MeleeRange * 0.5f);
    //    bool bHit = GetWorld()->SweepMultiByChannel(
    //        Hits,
    //        Start,
    //        End,
    //        FQuat::Identity,
    //        ECC_Pawn,              // or a custom channel
    //        Shape
    //    );

    //    if (bHit)
    //    {
    //        for (auto& Hit : Hits)
    //        {
    //            AActor* Other = Hit.GetActor();
    //            if (Other && Other != this)
    //            {
    //                // Apply damage
    //                UGameplayStatics::ApplyDamage(
    //                    Other,
    //                    CharacterStats.MeleeDamage,
    //                    GetController(),
    //                    this,
    //                    UDamageType::StaticClass()
    //                );
    //            }
    //        }
    //    }
    //}


    // Set timer to reset melee
    GetWorld()->GetTimerManager().SetTimer(
        MeleeTimerHandle,
        this,
        &ABaseMagicCharacter::ResetMelee,
        CharacterStats.MeleeCooldown,
        false
    );
}

void ABaseMagicCharacter::SetBulletTypeByIndex(int32 NewIndex)
{
    if (AvailableBulletTypes.IsValidIndex(NewIndex))
    {
        CurrentBulletTypeIndex = NewIndex;
        BulletToSpawn = AvailableBulletTypes[NewIndex];

        if (GEngine)
        {
            FString Msg = FString::Printf(
                TEXT("[%s] Selected bullet #%d: %s"),
                *GetName(),
                NewIndex,
                *BulletToSpawn->GetName()
            );
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, Msg);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid bullet index %d"), *GetName(), NewIndex);
    }
}

void ABaseMagicCharacter::OnStartShooting()
{
	isShooting = true;
}

void ABaseMagicCharacter::OnStopShooting()
{
	isShooting = false;
}

void ABaseMagicCharacter::OnStartMelee()
{
    isMeleeAttacking = true;
}

void ABaseMagicCharacter::OnStopMelee()
{
    isMeleeAttacking = false;
}

void ABaseMagicCharacter::ResetMelee()
{
    bCanMelee = true;
    isMeleeAttacking = false;
    OnMeleeStart.Broadcast(false);

    // **This is the signal your BT task is waiting on**
    OnMeleeFinished.Broadcast(false);
}

FVector ABaseMagicCharacter::CalculateMovementBlending()
{
	FVector movement = moveRotation.Vector();
	FVector shooting = shootRotation.Vector();

	float dotProduct = FVector::DotProduct(movement, shooting);

	FVector blendVector = movement - shooting * dotProduct;

	FVector outputVector = FVector(blendVector.Length(), dotProduct, 0);

	return outputVector * 100;
}

