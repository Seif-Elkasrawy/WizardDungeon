// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerCharacter.h"
#include "AOESpell.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Components/DecalComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/CapsuleComponent.h"




ABasePlayerCharacter::ABasePlayerCharacter()
{
	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

    
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 8.0f;       // higher = faster catch-up; try 6..12
    CameraBoom->CameraLagMaxDistance = 1000.0f; // optional clamp how far it can lag
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 10.0f;


	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    DissolveNiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DissolveNiagara"));
    if (DissolveNiagaraComp)
    {
        // Attach to mesh so it follows animations
        DissolveNiagaraComp->SetupAttachment(GetMesh());
        DissolveNiagaraComp->bAutoActivate = false;
        DissolveNiagaraComp->SetAutoDestroy(false); // keep component, we reuse it
    }
}

void ABasePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (bChargingArc || isShooting)
    {
        const float YawTolerance = 1.0f; // degrees
        float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, shootRotation.Yaw));
        if (DeltaYaw > YawTolerance)
        {
            SetActorRotation(shootRotation);
        }
    }
    else
    {
        SetActorRotation(moveRotation);
    }

    if (bChargingArc)
    {
        CurrentChargeTime = FMath::Min(CurrentChargeTime + DeltaTime, MaxChargeTime);
        // you can update a UI bar here using CurrentChargeTime / MaxChargeTime

                // update preview velocity every frame
        

        //if (GEngine)
        //{
        //    FString Msg = FString::Printf(
        //        TEXT("Charging arc: %.1f%%"),
        //        (CurrentChargeTime / MaxChargeTime) * 100.0f
        //    );
        //    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, Msg);
        //}
        PendingArcVelocity = ArcCharging();
        UpdateArcPreview();
    }
    
}

// Called to bind functionality to input
void ABasePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABasePlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (DissolveNiagaraComp && DissolveNiagaraSystem)
    {
        DissolveNiagaraComp->SetAsset(DissolveNiagaraSystem);
    }

    // create full-screen HUD (only locally)
    if (Player_HUDClass && IsLocallyControlled())
    {
        PlayerHUDWidget = CreateWidget<UPlayer_HUD>(GetWorld(), Player_HUDClass);
        if (PlayerHUDWidget)
        {
            PlayerHUDWidget->AddToViewport();

                float InitFrac = (CharacterStats.MaxHP > 0.f) ? (CharacterStats.HP / CharacterStats.MaxHP) : 0.f;
                PlayerHUDWidget->SetPlayerHPFraction(InitFrac);
            
        }
    }
}

float ABasePlayerCharacter::TakeDamage(float DamageCount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Let base class handle HP change, hit flash, death bookkeeping, etc.
    float Applied = Super::TakeDamage(DamageCount, DamageEvent, EventInstigator, DamageCauser);

    // Update HUD with new fraction
    if (PlayerHUDWidget)
    {
        float NewFraction = 0.f;
        if (CharacterStats.MaxHP > 0.f)
            NewFraction = CharacterStats.HP / CharacterStats.MaxHP;

        PlayerHUDWidget->SetPlayerHPFraction(NewFraction);
    }

    return Applied;
}

void ABasePlayerCharacter::OnStartArcCharge()
{
    // print bChargingArc to screen    
    //if (GEngine)
    //{
    //    FString Msg = FString::Printf(TEXT("OnStartArcCharge before return bChargingArc: %s"), bChargingArc ? TEXT("true") : TEXT("false"));
    //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
    //}

    // Start charging an arc to throw
    if (bChargingArc) return;
    bChargingArc = true;
    CurrentChargeTime = 0.f;
    PendingArcVelocity = FVector::ZeroVector;

}

FVector ABasePlayerCharacter::ArcCharging()
{

    float chargeRatio = CurrentChargeTime / MaxChargeTime;
    float desiredRange = FMath::Lerp(0.f, MaxArcRange, chargeRatio);

    // Use 45° launch for maximum distance:  
    // Range = v² / g  ⇒  v = sqrt(Range * g)
    float launchSpeed = FMath::Sqrt(desiredRange * ProjectileGravity);

    // Direction: flatten your facing to XY plane
    FVector Forward = shootRotation.Vector();
    Forward.Z = 0;
    Forward.Normalize();

    // Decompose into a 45° arc: cos45=sin45=0.707
    float comp = FMath::Sqrt(0.5f) * launchSpeed;
    FVector Velocity = Forward * comp + FVector(0, 0, comp);

    //if (GEngine) {
    //    FString Msg = FString::Printf(
    //        TEXT("Arc throw: Velocity: %s, Launch Speed: %.1f"),
    //        *Velocity.ToString(),
    //        launchSpeed
    //    );
    //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
    //}

    return Velocity;
}

void ABasePlayerCharacter::OnReleaseArcThrow()
{
    // Release the charged arc to throw it
    if (!bChargingArc) return;
    bChargingArc = false;

    HideLandingIndicator();
    // Use the last-computed velocity (or compute fresh to be safe)
    FVector VelocityToFire = PendingArcVelocity;
    if (VelocityToFire.IsNearlyZero())
    {
        // fallback — recompute from current charge
        VelocityToFire = ArcCharging();
    }
    // Broadcast and return
    ShootBullet(VelocityToFire);
}

void ABasePlayerCharacter::UpdateArcPreview()
{
    if (!SpawnLocation) return;

    FVector Start = SpawnLocation->GetComponentLocation();
    FVector LaunchVelocity = PendingArcVelocity; // computed in Tick()
    const float SimTime = 5.0f;          // how long to simulate max
    const float SimFreq = 30.0f;         // simulation steps per second
    const float Radius = 10.0f;         // projectile radius for collision checks

    FPredictProjectilePathParams Params;
    Params.StartLocation = Start;
    Params.LaunchVelocity = LaunchVelocity;
    Params.bTraceWithCollision = true;
    Params.ProjectileRadius = Radius;
    Params.MaxSimTime = SimTime;
    Params.SimFrequency = SimFreq;
    Params.OverrideGravityZ = -ProjectileGravity; // if your ProjectileGravity is positive 980
    Params.TraceChannel = ECC_Visibility; // or ECC_WorldStatic (whatever your collision uses)

    FPredictProjectilePathResult Result;
    bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

    // Quick: draw lines between consecutive samples
    const TArray<FPredictProjectilePathPointData>& Path = Result.PathData;
    for (int32 i = 1; i < Path.Num(); ++i)
    {
        const FVector& P0 = Path[i - 1].Location;
        const FVector& P1 = Path[i].Location;
        DrawDebugLine(GetWorld(), P0, P1, FColor::FromHex("00E7DAFF"), false, 0.05f, 0, 4.5f);
    }

    FVector LandPoint;
    if (bHit && Result.HitResult.IsValidBlockingHit())
    {
        LandPoint = Result.HitResult.Location;
    }
    else
    {
        // fallback to last simulated location
        LandPoint = Result.LastTraceDestination.Location;
    }

    // after PredictProjectilePathResult computed LandPoint and Normal...
    float ExplosionRadius = 200.f;
    if (AOESpellClass)
    {
        if (const AAOESpell* CDO = Cast<AAOESpell>(AOESpellClass->GetDefaultObject()))
            ExplosionRadius = CDO->ExplosionRadius;
    }

    // Save or directly update indicator actor location
    UpdateLandingIndicator(LandPoint, Result.HitResult.ImpactNormal, ExplosionRadius);
}

void ABasePlayerCharacter::UpdateLandingIndicator(const FVector& Location, const FVector& Normal, float ExplosionRadius)
{
    if (!LandingIndicatorClass) return;

    // spawn once if needed
    if (!LandingIndicatorActor)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        LandingIndicatorActor = GetWorld()->SpawnActor<AActor>(LandingIndicatorClass, Location, FRotator::ZeroRotator, Params);
        if (!LandingIndicatorActor) return;
    }

    // position slightly above surface to avoid z-fighting
    const float Offset = 4.0f;
    FVector TargetLoc = Location + Normal * Offset;
    LandingIndicatorActor->SetActorLocation(TargetLoc);

    // rotate to align with surface (optional)
    FRotator Rot = Normal.Rotation();
    // we want the mesh upright, so zero pitch/roll if needed
    Rot.Roll = 0.f;
    Rot.Pitch = 0.f;
    LandingIndicatorActor->SetActorRotation(Rot);

    // scale the indicator so its *radius* equals ExplosionRadius
    float ScaleFactor = FMath::Max(0.01f, ExplosionRadius / IndicatorMeshDefaultRadius);
    LandingIndicatorActor->SetActorScale3D(FVector(ScaleFactor));

    // ensure visible
    LandingIndicatorActor->SetActorHiddenInGame(false);
}

void ABasePlayerCharacter::HideLandingIndicator()
{

	// Hide the landing indicator actor
    if (!LandingIndicatorActor) return;
    // hide and keep around for reuse
    LandingIndicatorActor->SetActorHiddenInGame(true);
}

void ABasePlayerCharacter::SetAimFromDirection(const FVector& Direction3D)
{
    if (Direction3D.IsNearlyZero()) return;

    FVector Dir = Direction3D;
    Dir.Z = 0.f;
    Dir.Normalize();

    FRotator FullRot = Dir.Rotation();
    shootRotation = FRotator(0.f, FullRot.Yaw, 0.f);

    // If currently charging or shooting, immediately apply it visually
    if (bChargingArc || isShooting)
    {
        SetActorRotation(shootRotation);
    }
}

void ABasePlayerCharacter::OnStartDodge()
{
    Dodge();
}

void ABasePlayerCharacter::OnStopDodge()
{
    // not used for instant teleport; kept for completeness
}

void ABasePlayerCharacter::Dodge()
{
    if (!bCanDodge) return;

    if (!Controller) Controller = GetController(); // optional ensure

    // Prevent dodging while dead / during actions if you need
    if (bIsDead) return;

    // compute forward 2D direction (yaw only)
    //FVector Forward = GetActorForwardVector();
    //Forward.Z = 0.f;
    //if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
    //Forward.Normalize();

    //const FVector Start = GetActorLocation();
    //const FVector Desired = Start + Forward * DodgeDistance;

    //// Setup sweep shape (capsule approximating character)
    //float SweepRadius = DodgeSweepRadius;
    //float SweepHalfHeight = DodgeSweepHalfHeight;

    //// make sure the shape somewhat aligns with the capsule component if present
    //UCapsuleComponent* MyCapsule = FindComponentByClass<UCapsuleComponent>();
    //if (MyCapsule)
    //{
    //    SweepRadius = FMath::Max(SweepRadius, MyCapsule->GetScaledCapsuleRadius());
    //    SweepHalfHeight = FMath::Max(SweepHalfHeight, MyCapsule->GetScaledCapsuleHalfHeight());
    //}

    //FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DodgeSweep), false, this);
    //QueryParams.bReturnPhysicalMaterial = false;
    //QueryParams.AddIgnoredActor(this);

    //FHitResult Hit;
    //bool bHit = GetWorld()->SweepSingleByChannel(
    //    Hit,
    //    Start,
    //    Desired,
    //    FQuat::Identity,
    //    ECC_Visibility, // or ECC_WorldStatic; pick the channel you use for blocking geometry
    //    FCollisionShape::MakeCapsule(SweepRadius, SweepHalfHeight),
    //    QueryParams
    //);

    //// compute FinalLocation (use existing sweep code)
    //FVector FinalLocation = Desired;
    //if (bHit && Hit.bBlockingHit)
    //{
    //    const float SafetyOffset = 10.0f; // cm back from hit
    //    FinalLocation = Hit.Location - Forward * SafetyOffset;
    //}

    //// store pending teleport and begin dissolve + VFX — do NOT teleport yet
    //PendingTeleportLocation = FinalLocation;
    bHasPendingTeleport = true;

    // start dissolve material animation
    StartDissolveOut();

    // start (or restart) the Niagara dissolve effect attached to mesh
    if (DissolveNiagaraComp)
    {
        if (DissolveNiagaraSystem)
            DissolveNiagaraComp->SetAsset(DissolveNiagaraSystem);

        // attach/snap (safe) to mesh and restart
        DissolveNiagaraComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
        DissolveNiagaraComp->SetRelativeLocation(FVector::ZeroVector);
        DissolveNiagaraComp->SetRelativeRotation(FRotator::ZeroRotator);

        DissolveNiagaraComp->Deactivate();    // safe reset
        DissolveNiagaraComp->ResetSystem();   // clear any particles from previous run
        DissolveNiagaraComp->Activate(true);  // start fresh
    }

    // Give brief invulnerability
    bIsDodgeInvulnerable = true;
    if (DodgeInvulnerabilityTime > 0.f)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_EndInvuln);
        GetWorldTimerManager().SetTimer(TimerHandle_EndInvuln, this, &ABasePlayerCharacter::EndDodgeInvuln, DodgeInvulnerabilityTime, false);
    }

    // Start cooldown
    bCanDodge = false;
    GetWorldTimerManager().ClearTimer(TimerHandle_DodgeCooldown);
    GetWorldTimerManager().SetTimer(TimerHandle_DodgeCooldown, this, &ABasePlayerCharacter::ResetDodge, DodgeCooldown, false);
}

void ABasePlayerCharacter::InitDissolveMIDs()
{
    // if already inited, skip
    if (DynamicMats.Num() > 0) return;

    USkeletalMeshComponent* Skel = GetMesh();
    if (!IsValid(Skel)) return;

    int32 NumMats = Skel->GetNumMaterials();
    DynamicMats.SetNum(NumMats);
    OriginalMats.SetNum(NumMats);

    for (int32 i = 0; i < NumMats; ++i)
    {
        UMaterialInterface* Mat = Skel->GetMaterial(i);
        OriginalMats[i] = Mat;

        if (Mat)
        {
            UMaterialInstanceDynamic* MID = Skel->CreateAndSetMaterialInstanceDynamic(i);
            DynamicMats[i] = MID;

            if (MID)
            {
                MID->SetScalarParameterValue(DissolveParameterName, 0.0f);
            }
        }
    }
}

void ABasePlayerCharacter::StartDissolveOut()
{
    InitDissolveMIDs();
    if (DynamicMats.Num() == 0) return;

    // start timer
    DissolveElapsed = 0.f;
    bDissolvingOut = true;

    GetWorldTimerManager().ClearTimer(TimerHandle_DissolveTick);
    // tick frequency 60hz (approx); you can use lower rate if you prefer
    GetWorldTimerManager().SetTimer(TimerHandle_DissolveTick, this, &ABasePlayerCharacter::UpdateDissolveTick, 1.0f / 60.0f, true);
}

void ABasePlayerCharacter::StartResolve()
{
    // assume MIDs already created by StartDissolveOut; if not, init
    InitDissolveMIDs();

    DissolveElapsed = 0.f;
    bDissolvingOut = false;

    GetWorldTimerManager().ClearTimer(TimerHandle_DissolveTick);
   // GetWorldTimerManager().SetTimer(TimerHandle_DissolveTick, this, &ABasePlayerCharacter::UpdateDissolveTick, 1.0f / 60.0f, true);
}

void ABasePlayerCharacter::UpdateDissolveTick()
{
    // delta per tick
    float Delta = 1.0f / 60.0f;
    DissolveElapsed += Delta;

    float Alpha;
    if (bDissolvingOut)
    {
        if (DissolveDuration <= 0.f) Alpha = 1.f;
        else Alpha = FMath::Clamp(DissolveElapsed / DissolveDuration, 0.f, 1.f);
    }
    else
    {
        if (ResolveDuration <= 0.f) Alpha = 0.f;
        else Alpha = 1.f - FMath::Clamp(DissolveElapsed / ResolveDuration, 0.f, 1.f);
    }

    for (UMaterialInstanceDynamic* MID : DynamicMats)
    {
        if (MID) MID->SetScalarParameterValue(DissolveParameterName, Alpha);
    }

    // finish
    if ((bDissolvingOut && DissolveElapsed >= DissolveDuration) ||
        (!bDissolvingOut && DissolveElapsed >= ResolveDuration))
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_DissolveTick);

        if (bDissolvingOut)
        {
            // teleport now (we were fully invisible)
            if (bHasPendingTeleport)
            {
                PerformTeleportNow();
                bHasPendingTeleport = false;
            }

            // start resolve (material 1->0)
            StartResolve();

        }
        else
        {
            // finished resolve: cleanup Niagara (stop and reset)
            if (DissolveNiagaraComp)
            {
                // ResetSystem + Deactivate ensures no leftover particles and stops looping systems
                DissolveNiagaraComp->Deactivate();
                DissolveNiagaraComp->ResetSystem();
            }

            // finished fully — make sure invulnerability is ended if not already
            // (we already set a timer for invuln, but you can clear or adjust it here if desired)
        }
    }

}

void ABasePlayerCharacter::OnDissolveNiagaraFinished(UNiagaraComponent* FinishedComponent)
{
}

void ABasePlayerCharacter::PerformTeleportNow()
{
    if (!bHasPendingTeleport) return;

    FVector Forward = GetActorForwardVector();
    Forward.Z = 0.f;
    if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
    Forward.Normalize();

    // recompute safe final location (optional) — this reuses the capsule size logic
    FVector Start = GetActorLocation();
    PendingTeleportLocation = Start + Forward * DodgeDistance;
    FVector RecomputedFinal = PendingTeleportLocation;

    // a quick sweep to be safe (you can factor this into a helper as discussed earlier)
    UCapsuleComponent* MyCapsule = FindComponentByClass<UCapsuleComponent>();
    float SweepRadius = DodgeSweepRadius;
    float SweepHalfHeight = DodgeSweepHalfHeight;
    if (MyCapsule)
    {
        SweepRadius = FMath::Max(SweepRadius, MyCapsule->GetScaledCapsuleRadius());
        SweepHalfHeight = FMath::Max(SweepHalfHeight, MyCapsule->GetScaledCapsuleHalfHeight());
    }

    FHitResult Hit;
    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        PendingTeleportLocation,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeCapsule(SweepRadius, SweepHalfHeight),
        FCollisionQueryParams(SCENE_QUERY_STAT(DodgeSweep), false, this));

    if (bHit && Hit.bBlockingHit)
    {
        const float SafetyOffset = 10.0f;
        FVector Dir = (PendingTeleportLocation - Start).GetSafeNormal();
        RecomputedFinal = Hit.Location - Dir * SafetyOffset;
    }

    // perform move with sweep so engine can resolve collisions
    FHitResult MoveHit;
    bool bMoved = SetActorLocation(RecomputedFinal, true /*bSweep*/, &MoveHit, ETeleportType::TeleportPhysics);

    if (!bMoved)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] PerformTeleportNow: final SetActorLocation failed, staying put."), *GetName());
    }
}


void ABasePlayerCharacter::ResetDodge()
{
    bCanDodge = true;
}

void ABasePlayerCharacter::EndDodgeInvuln()
{
    bIsDodgeInvulnerable = false;
}

