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




ABasePlayerCharacter::ABasePlayerCharacter()
{
	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
    TrajectorySpline->SetupAttachment(RootComponent);
    TrajectorySpline->SetMobility(EComponentMobility::Movable);

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

    // print bChargingArc to screen    
    //if (GEngine)
    //{
    //    FString Msg = FString::Printf(TEXT("OnStartArcCharge after return bChargingArc: %s"), bChargingArc ? TEXT("true") : TEXT("false"));
    //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
    //}

    //// you might set a flag or start a timer
    //if (GEngine)
    //{
    //    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, TEXT("Arc throw started!"));
    //}

}

FVector ABasePlayerCharacter::ArcCharging()
{
    // print bChargingArc to screen    
    //if (GEngine)
    //{
    //    FString Msg = FString::Printf(TEXT("OnReleaseArcThrow before return bChargingArc: %s"), bChargingArc ? TEXT("true") : TEXT("false"));
    //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
    //}

    //if (GEngine)
    //{
    //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Arc throw released!"));
    //}

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

    //// Clear existing spline points
    //TrajectorySpline->ClearSplinePoints();

    //// Add points to spline
    //for (int32 i = 0; i < Result.PathData.Num(); ++i)
    //{
    //    FVector WorldLoc = Result.PathData[i].Location;
    //    TrajectorySpline->AddSplineWorldPoint(WorldLoc);
    //}
    //TrajectorySpline->UpdateSpline();

    //// Optionally, create/update spline meshes to render a tube
    //UpdateSplineMeshesFromSpline();


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

//void ABasePlayerCharacter::UpdateSplineMeshesFromSpline()
//{
//    if (!TrajectorySpline) return;
//
//    const int32 NumPoints = TrajectorySpline->GetNumberOfSplinePoints();
//    const int32 NumSegments = FMath::Max(0, NumPoints - 1);
//
//    // Ensure we have enough spline mesh components
//    for (int32 i = TrajectorySplineMeshes.Num(); i < NumSegments; ++i)
//    {
//        // Create a new spline mesh component
//        USplineMeshComponent* S = NewObject<USplineMeshComponent>(this);
//        if (!S) continue;
//
//        // Important: set mobility before registering
//        S->SetMobility(EComponentMobility::Movable);
//
//        // Attach to the spline component so its local space matches the spline
//        S->AttachToComponent(TrajectorySpline, FAttachmentTransformRules::KeepRelativeTransform);
//
//        // Disable collision for preview meshes (avoid blocking & warnings)
//        S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//        S->SetGenerateOverlapEvents(false);
//        S->SetVisibility(true);
//
//        // Set mesh/material only if provided
//        if (SplineMesh)
//        {
//            S->SetStaticMesh(SplineMesh);
//        }
//
//        // Register the component so it becomes part of the world
//        S->RegisterComponent();
//
//        TrajectorySplineMeshes.Add(S);
//    }
//
//    // Update existing segments
//    for (int32 i = 0; i < NumSegments; ++i)
//    {
//        USplineMeshComponent* S = TrajectorySplineMeshes.IsValidIndex(i) ? TrajectorySplineMeshes[i] : nullptr;
//        if (!S) continue;
//
//        // Get start/end pos & tangents in LOCAL space so mesh coords line up with spline's local transform
//        FVector StartPos = TrajectorySpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
//        FVector StartTangent = TrajectorySpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
//        FVector EndPos = TrajectorySpline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
//        FVector EndTangent = TrajectorySpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
//
//        // Fallback tangents if points produce very small tangents (prevents degenerate geometry)
//        if (StartTangent.IsNearlyZero())
//        {
//            StartTangent = (EndPos - StartPos) * 0.5f;
//        }
//        if (EndTangent.IsNearlyZero())
//        {
//            EndTangent = (EndPos - StartPos) * 0.5f;
//        }
//
//        // Forward axis must match your mesh orientation.
//        // For UE cylinder asset try Z. For a thin box aligned on X, use X.
//        S->SetForwardAxis(ESplineMeshAxis::X);
//
//        // Set geometry (local space coords because S is attached to spline)
//        S->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);
//
//        // Make it visible & set thickness
//        S->SetStartScale(FVector2D(0.2f, 0.2f));
//        S->SetEndScale(FVector2D(0.2f, 0.2f));
//        S->SetVisibility(true);
//    }
//
//    // Hide / disable unused spline meshes
//    for (int32 i = NumSegments; i < TrajectorySplineMeshes.Num(); ++i)
//    {
//        if (TrajectorySplineMeshes[i])
//            TrajectorySplineMeshes[i]->SetVisibility(false);
//    }
//}

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
    //// Clear spline points
    //if (TrajectorySpline)
    //{
    //    TrajectorySpline->ClearSplinePoints();
    //    TrajectorySpline->UpdateSpline();
    //}

    //// Destroy spline mesh components and clear array (you can pool instead of destroy if desired)
    //for (USplineMeshComponent* Comp : TrajectorySplineMeshes)
    //{
    //    if (Comp)
    //    {
    //        Comp->DestroyComponent();
    //    }
    //}
    //TrajectorySplineMeshes.Empty();

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
