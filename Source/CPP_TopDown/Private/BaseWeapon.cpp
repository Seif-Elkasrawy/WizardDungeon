// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon Mesh"));

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	HitCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("HitCollider"));
    HitCollider->SetupAttachment(WeaponMesh, TEXT("CollisionSocket"));

    // default collision: world dynamic, overlap pawns
    HitCollider->SetCollisionObjectType(ECC_WorldDynamic);
    HitCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // don't generate overlaps until we enable melee
    HitCollider->SetGenerateOverlapEvents(false);
    HitCollider->SetNotifyRigidBodyCollision(false);
}

void ABaseWeapon::StartMeleeWindow(float WindowSeconds)
{
    HitActors.Empty();

    // ensure owner is ignored so we don't hit ourself
    if (Player)
    {
        HitCollider->IgnoreActorWhenMoving(Player, true);
    }

    HitCollider->SetGenerateOverlapEvents(true);

    // schedule stop
    if (WindowSeconds > 0.f)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_StopWindow);
        GetWorldTimerManager().SetTimer(TimerHandle_StopWindow, this, &ABaseWeapon::StopMeleeWindow, WindowSeconds, false);
    }
}

void ABaseWeapon::StopMeleeWindow()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_StopWindow);
    HitCollider->SetGenerateOverlapEvents(false);

    // stop ignoring owner if you changed it (optional)
    if (Player)
    {
        HitCollider->IgnoreActorWhenMoving(Player, false);
    }
}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

    // bind once (we will enable/disable generation only)
    HitCollider->OnComponentBeginOverlap.AddDynamic(this, &ABaseWeapon::OnHitOverlap);
}

void ABaseWeapon::WeaponShoot()
{

}

void ABaseWeapon::SetPlayerPointer(ACharacter* PlayerPtr)
{
	Player = PlayerPtr;
}


void ABaseWeapon::OnHitOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == Player || OtherActor == this) return;

    // avoid hitting same actor multiple times during same window
    if (HitActors.Contains(OtherActor)) return;

    HitActors.Add(OtherActor);

    // Apply damage - choose Instigator Controller (owner pawn's controller) if possible
    AController* InstigatorController = nullptr;
    if (Player)
    {
        InstigatorController = Player->GetController();
    }

    UGameplayStatics::ApplyDamage(OtherActor, MeleeDamage, InstigatorController, this, UDamageType::StaticClass());

    // OPTIONAL: spawn hit fx, play sound, etc.
}

// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

