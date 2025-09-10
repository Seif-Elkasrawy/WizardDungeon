// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseBullet.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ABaseBullet::ABaseBullet()
{

	collisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	SetRootComponent(collisionSphere);

    BulletFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Effects"));
	BulletFX->SetupAttachment(collisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0;
    ProjectileMovement->UpdatedComponent = collisionSphere;
    //ProjectileMovement->Velocity = GetActorForwardVector() * speed;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    // default lifespan for pooled bullets (optional)
    LifeSpan = 5.0f;

}

// Called when the game starts or when spawned
void ABaseBullet::BeginPlay()
{
	Super::BeginPlay();

	collisionSphere->OnComponentHit.AddDynamic(this, &ABaseBullet::OnComponentHit);
	
}

void ABaseBullet::InitializeBullet(APawn* InInstigator, const FVector& Velocity)
{
    SetOwner(InInstigator);
    SetInstigator(InInstigator);

    // 1) Ignore the pawn that fired us
    if (AActor* MyOwner = GetOwner())
    {
        collisionSphere->IgnoreActorWhenMoving(MyOwner, true);

        // 2) Also ignore any attached actors (e.g. the weapon mesh child actor)
        TArray<AActor*> Attached;
        MyOwner->GetAttachedActors(Attached);
        for (AActor* Child : Attached)
        {
            collisionSphere->IgnoreActorWhenMoving(Child, true);
        }
    }

    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = Velocity;
		ProjectileMovement->Activate(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeBullet: ProjectileMovement is null"));
    }

    if (BulletFX && !BulletFX->IsActive())
    {
        BulletFX->Activate(true);
    }
    else if (!BulletFX)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeBullet: BulletFX is null"));
	}

	collisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SetInUse(true);
}

void ABaseBullet::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 1) Make sure we're hitting something valid
    if (!OtherActor || OtherActor == this)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnComponentHit: invalid OtherActor"));
        return;
    }

    BulletHit(OtherActor, Hit);

    ReturnToPool();
}

void ABaseBullet::BulletHit(AActor* OtherActor, const FHitResult& Hit)
{
    // Sanity checks
    if (!IsValid(OtherActor) || OtherActor == this)
    {
        return;
    }

    // optional: don't hit the pawn that fired the bullet
    if (OtherActor == GetInstigator())
    {
        return;
    }

    // Spawn impact FX only if the system is set
    if (ImpactParticles)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            ImpactParticles,
            GetActorLocation());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("OnComponentHit: ImpactParticles is null"));
    }

    // 4) Get a valid controller from the instigator
    AController* InstigatorController = GetInstigator()
		? GetInstigatorController()
        : nullptr;
    if (!InstigatorController)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnComponentHit: no valid instigator/controller"));
    }

    // 5) Finally apply damage—only if we have a DamageType class
    if (DamageType && IsValid(OtherActor))
    {

        // IMPORTANT: ApplyDamage may call TakeDamage() on OtherActor which can
        // destroy it synchronously. Do NOT access OtherActor after this call.
        UGameplayStatics::ApplyDamage(
            OtherActor,
            baseDamage,
            InstigatorController,
            this,
            DamageType);
    }
    else if (!DamageType)
    {
        UE_LOG(LogTemp, Warning, TEXT("BulletHit: DamageType is null"));
    }

}

void ABaseBullet::ReturnToPool()
{
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Deactivate();
    }
    if (BulletFX && BulletFX->IsActive())
    {
        BulletFX->Deactivate();
    }
    //collisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // reset owner/instigator
    SetOwner(nullptr);
    SetInstigator(nullptr);

    APooledActor::ReturnToPool();
}

// Called every frame
void ABaseBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

