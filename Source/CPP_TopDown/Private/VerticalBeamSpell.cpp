// Fill out your copyright notice in the Description page of Project Settings.


#include "VerticalBeamSpell.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include <BaseMagicCharacter.h>
#include "GameFramework/DamageType.h"  
#include "Engine/DamageEvents.h"  

// Sets default values
AVerticalBeamSpell::AVerticalBeamSpell()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Create and configure collider (we'll register it at runtime when initializing)
    Collider = CreateDefaultSubobject<UBoxComponent>(TEXT("BeamCollider"));
    Collider->SetGenerateOverlapEvents(false);
    // Collider->SetBoxExtent(FVector(100.f, 100.f, 500.f)); // safe defaults, will be reconfigured
    RootComponent = Collider;

    // Create a Niagara component as part of the actor so you can edit/preview in BP
    NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamNiagara"));
    NiagaraComp->SetupAttachment(RootComponent);

    // Don't auto-activate; activate from Initialize() so you control timing at runtime.
    NiagaraComp->SetAutoActivate(false);

    Lifetime = 2.0f;
    Scale = 1.0f;
    DPS = 20.0f;
    DamageTickInterval = 0.2f;

}

void AVerticalBeamSpell::Initialize(AActor* TargetActor, bool bAttachToTarget, float Duration, float BeamScale, float DamagePerSecond, float TickInterval)
{
    if (!TargetActor)
    {
        Destroy();
        return;
    }

    Lifetime = Duration;
    Scale = FMath::Max(0.01f, BeamScale);
    DPS = DamagePerSecond;
    DamageTickInterval = FMath::Max(0.01f, TickInterval);

    // If the designer assigned BeamFX in BP, set it on the component now (safe to do at runtime).
    if (BeamFX && NiagaraComp)
    {
        NiagaraComp->SetAsset(BeamFX);
    }

    // Spawn Niagara (attached or at world location)
    FVector SpawnLocation = TargetActor->GetActorLocation();
    FRotator SpawnRotation = FRotator::ZeroRotator;

    if (BeamFX)
    {
        if (bAttachToTarget)
        {
            NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
                BeamFX,
                TargetActor->GetRootComponent(),
                NAME_None,
                FVector::ZeroVector,
                SpawnRotation,
                EAttachLocation::KeepRelativeOffset,
                true,
                true
            );
            if (NiagaraComp)
            {
                NiagaraComp->SetRelativeScale3D(FVector(Scale));
            }
        }
        else
        {
            NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BeamFX, SpawnLocation, SpawnRotation);
            if (NiagaraComp)
            {
                NiagaraComp->SetWorldScale3D(FVector(Scale));
                // Activate the system now (if you want it to play immediately)
                NiagaraComp->Activate(true);
            }
        }
    }

    // Configure collider extents & enable overlap
    // We'll attach collider to target so it moves with them if bAttachToTarget is true.
    if (bAttachToTarget)
    {
        // attach this actor to the target so both Niagara and collider follow it
        AttachToActor(TargetActor, FAttachmentTransformRules::KeepWorldTransform);
        SetActorRelativeLocation(FVector::ZeroVector);
    }
    else
    {
        SetActorLocation(SpawnLocation);
    }

    if (DamageStartDelay <= 0.0f)
    {
        StartDamagePhase();
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimer(DamageStartTimerHandle, this, &AVerticalBeamSpell::StartDamagePhase, DamageStartDelay, false);
    }

    // Schedule beam end
    if (Lifetime > 0.f)
    {
        GetWorld()->GetTimerManager().SetTimer(LifeTimer, this, &AVerticalBeamSpell::EndBeam, Lifetime, false);
    }
}

// Called when the game starts or when spawned
void AVerticalBeamSpell::BeginPlay()
{
	Super::BeginPlay();
	
    // If BeamFX was assigned in C++ defaults (rare), you can set it here:
    if (BeamFX && NiagaraComp)
    {
        NiagaraComp->SetAsset(BeamFX);
    }
}

void AVerticalBeamSpell::StartDamagePhase()
{
    // Safety
    if (!IsValid(Collider)) return;

    Collider->SetGenerateOverlapEvents(true);
    Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // Register the component (if not already)
    if (!Collider->IsRegistered())
    {
        Collider->RegisterComponent();
    }

    // Bind overlap delegates
    Collider->OnComponentBeginOverlap.AddDynamic(this, &AVerticalBeamSpell::OnBeginOverlap);
    Collider->OnComponentEndOverlap.AddDynamic(this, &AVerticalBeamSpell::OnEndOverlap);

    Collider->UpdateOverlaps();            // force overlap detection update

    // Add actors already inside the collider (very important if they spawn inside)
    TArray<AActor*> InitiallyOverlapping;
    Collider->GetOverlappingActors(InitiallyOverlapping);
    for (AActor* A : InitiallyOverlapping)
    {
        if (IsValid(A) && A != GetInstigator())
        {
            OverlappingActors.Add(A);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                    FString::Printf(TEXT("Initial overlap: %s"), *GetNameSafe(A)));
            }
        }
    }

    // Start periodic damage timer
    if (!GetWorld()->GetTimerManager().IsTimerActive(DamageTickTimer))
    {
        GetWorld()->GetTimerManager().SetTimer(DamageTickTimer, this, &AVerticalBeamSpell::ApplyDamageTick, DamageTickInterval, true);
    }
}

void AVerticalBeamSpell::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

    if (!OtherActor || OtherActor == GetInstigator()) return;
    OverlappingActors.Add(OtherActor);

    UE_LOG(LogTemp, Log, TEXT("Beam OnBeginOverlap: %s overlapped by %s"), *GetName(), *GetNameSafe(OtherActor));
}

void AVerticalBeamSpell::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor) return;
    OverlappingActors.Remove(OtherActor);
}

void AVerticalBeamSpell::ApplyDamageTick()
{

    if (OverlappingActors.Num() == 0) return;

    // damage per tick = DPS * tick interval
    const float DamagePerTick = DPS * DamageTickInterval;

    // choose instigator controller from owner if available
    AController* InstigatorController = nullptr;
    if (AActor* OwnerActor = GetOwner())
    {
        if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
        {
            InstigatorController = OwnerPawn->GetController();
        }
    }

    TArray<TWeakObjectPtr<AActor>> ToRemove;
    for (auto WeakA : OverlappingActors)
    {
        AActor* Actor = WeakA.Get();
        if (!IsValid(Actor))
        {
            ToRemove.Add(WeakA);
            continue;
        }

        // Optionally restrict to pawns/characters:
        //APawn* P = Cast<APawn>(Actor);
        //if (!P) continue;

        // Debug: print actor info so we know who we hit
        //if (GEngine)
        //{
        //    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue,
        //        FString::Printf(TEXT("Beam about to damage %s for %.1f"), *GetNameSafe(Actor), DamagePerTick));
        //}

		// Apply damage to the actor
        UGameplayStatics::ApplyDamage(Actor, DamagePerTick, InstigatorController, this, UDamageType::StaticClass());

        // Optionally: if you expect immediate effect and want to be extra sure, call TakeDamage directly
        // (only for debugging; remove in production)
        //if (ABaseMagicCharacter* M = Cast<ABaseMagicCharacter>(Actor))
        //{
        //    M->TakeDamage(DamagePerTick, FDamageEvent(), InstigatorController, this);
        //}
    }

    for (auto& R : ToRemove) OverlappingActors.Remove(R);
}

void AVerticalBeamSpell::EndBeam()
{
    // Stop timers
    GetWorld()->GetTimerManager().ClearTimer(DamageStartTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(DamageTickTimer);
    GetWorld()->GetTimerManager().ClearTimer(LifeTimer);

    // Unbind
    if (Collider)
    {
        Collider->OnComponentBeginOverlap.RemoveAll(this);
        Collider->OnComponentEndOverlap.RemoveAll(this);
    }

    OverlappingActors.Empty();

    // destroy Niagara component if present
    if (IsValid(NiagaraComp))
    {
        NiagaraComp->DestroyComponent();
        NiagaraComp = nullptr;
    }

    // Destroy this actor (will also remove collider)
    Destroy();
}

void AVerticalBeamSpell::ConfigureCollider()
{
    // Beam geometry: small XY radius, tall Z height (scale-controlled).
// Tune these constants as needed.
    const float BaseRadius = 100.f;
    const float BaseHeight = 1200.f;

    const float Radius = BaseRadius * Scale;
    const float Height = BaseHeight * Scale;

    // Box extent uses half-sizes
    Collider->SetBoxExtent(FVector(Radius, Radius, Height * 0.5f));
    // Keep collider's center at actor origin so beam is centered on actor
    Collider->SetRelativeLocation(FVector::ZeroVector);
}

