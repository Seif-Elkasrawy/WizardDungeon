    // Fill out your copyright notice in the Description page of Project Settings.


    #include "BaseEnemyCharacter.h"
    #include "NiagaraFunctionLibrary.h"
    #include "NiagaraComponent.h"
    #include "Components/BoxComponent.h"
    #include "Components/WidgetComponent.h"

    ABaseEnemyCharacter::ABaseEnemyCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;

        // Create widget component and set sane defaults
        HealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
        HealthWidgetComponent->SetupAttachment(RootComponent);

        // sensible defaults for world-space HP bar
        HealthWidgetComponent->SetWidgetSpace(EWidgetSpace::World);         // world-space
        HealthWidgetComponent->SetDrawSize(FVector2D(300.f, 32.f));         // visible size
        HealthWidgetComponent->SetPivot(FVector2D(0.5f, 0.0f));            // top-center pivot
        HealthWidgetComponent->SetTwoSided(true);
        HealthWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HealthWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 160.f)); // above head (tweak)
        HealthWidgetComponent->SetVisibility(true);
        HealthWidgetComponent->SetTickWhenOffscreen(true);
    }

    void ABaseEnemyCharacter::BeginPlay()
    {
        Super::BeginPlay();

        // debug at very start of BeginPlay() (or right after Super::BeginPlay())
        //UE_LOG(LogTemp, Log, TEXT("[%s] BeginPlay debug: HealthWidgetComponent=%s, IsRegistered=%s, WidgetClass=%s, RawWidget=%s, Location=%s"),
        //    *GetName(),
        //    *GetNameSafe(HealthWidgetComponent),
        //    (HealthWidgetComponent ? (HealthWidgetComponent->IsRegistered() ? TEXT("true") : TEXT("false")) : TEXT("n/a")),
        //    (EnemyHealthWidgetClass ? *EnemyHealthWidgetClass->GetName() : TEXT("null")),
        //    *(GetNameSafe(HealthWidgetComponent ? HealthWidgetComponent->GetUserWidgetObject() : nullptr)),
        //    *GetActorLocation().ToCompactString()
        //);

        // --- Init the health widget (hardened & verbose) ---
        UE_LOG(LogTemp, Log, TEXT("%s::BeginPlay() - starting widget init"), *GetName());

        if (!IsValid(HealthWidgetComponent))
        {
            UE_LOG(LogTemp, Error, TEXT("%s: HealthWidgetComponent is null or invalid!"), *GetName());
            return;
        }

        // Make sure component is registered (usually it is, but this will be defensive)
        if (!HealthWidgetComponent->IsRegistered())
        {
            HealthWidgetComponent->RegisterComponent();
            UE_LOG(LogTemp, Log, TEXT("%s: HealthWidgetComponent was not registered; RegisterComponent() called."), *GetName());
        }

        // Check the class you want to assign
        if (EnemyHealthWidgetClass == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: EnemyHealthWidgetClass is null - no widget will be created."), *GetName());
            return;
        }

        // Log the widget class name
        UE_LOG(LogTemp, Log, TEXT("%s: EnemyHealthWidgetClass = %s"), *GetName(), *EnemyHealthWidgetClass->GetName());

        // Set the Widget class on the component but DO NOT force InitWidget() yet.
        // Let UWidgetComponent create the widget when it's safe.
        HealthWidgetComponent->SetWidgetClass(EnemyHealthWidgetClass);

        // Try to get existing widget instance and init safely
        UUserWidget* RawWidget = HealthWidgetComponent->GetUserWidgetObject();
        if (!RawWidget)
        {
            HealthWidgetComponent->InitWidget();
            RawWidget = HealthWidgetComponent->GetUserWidgetObject();
        }

        UE_LOG(LogTemp, Log, TEXT("[%s] Raw widget created: %s (class: %s)"),
            *GetName(),
            *GetNameSafe(RawWidget),
            *GetNameSafe(RawWidget ? RawWidget->GetClass() : nullptr));

        // cache raw
        EnemyRawWidgetInstance = RawWidget;

        // try typed cast
        EnemyHPWidgetInstance = RawWidget ? Cast<UHPWidgetBase>(RawWidget) : nullptr;

        if (EnemyHPWidgetInstance)
        {
            UE_LOG(LogTemp, Log, TEXT("[%s] EnemyHPWidgetInstance valid: %p, widget class = %s"),
                *GetName(), EnemyHPWidgetInstance, *EnemyHPWidgetInstance->GetClass()->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] EnemyHPWidgetInstance NULL — raw widget class = %s. Fallback will be used."),
                *GetName(), *GetNameSafe(RawWidget ? RawWidget->GetClass() : nullptr));
        }

        // Initialize displayed value using whichever we have
        float InitFrac = (CharacterStats.MaxHP > 0.f) ? (CharacterStats.HP / CharacterStats.MaxHP) : 0.f;
        if (EnemyHPWidgetInstance)
        {
            EnemyHPWidgetInstance->SetHPFraction(InitFrac);
        }
        else if (EnemyRawWidgetInstance)
        {
            // try to call Blueprint function by name as fallback
            FName FuncName = TEXT("SetHPFraction");
            UFunction* Func = EnemyRawWidgetInstance->FindFunction(FuncName);
            if (Func)
            {
                struct FParam { float NewFraction; };
                FParam P{ InitFrac };
                EnemyRawWidgetInstance->ProcessEvent(Func, &P);
            }
        }

        // after you try to create/cast the widget
        UE_LOG(LogTemp, Log, TEXT("[%s] WidgetClass: %s, RawWidget: %s"),
            *GetName(),
            (EnemyHealthWidgetClass ? *EnemyHealthWidgetClass->GetName() : TEXT("null")),
            *GetNameSafe(RawWidget));

        if (EnemyHPWidgetInstance)
        {
            UE_LOG(LogTemp, Log, TEXT("[%s] EnemyHPWidgetInstance valid: %p, widget class = %s"),
                *GetName(),
                EnemyHPWidgetInstance,
                *EnemyHPWidgetInstance->GetClass()->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] EnemyHPWidgetInstance is NULL! Raw widget class = %s"),
                *GetName(),
                *GetNameSafe(RawWidget ? RawWidget->GetClass() : nullptr));
        }

        if (!PatrolRouteActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s has no PatrolRouteActor assigned!"), *GetName());
            return;
        }

        // --- 2) Patrol/spline logic AFTER widget init ---
        if (!PatrolRouteActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s has no PatrolRouteActor assigned!"), *GetName());
            // DO NOT return here — we still want widget and other setup to complete
        }
        else
        {
            SplineComp = PatrolRouteActor->FindComponentByClass<USplineComponent>();
            if (!SplineComp)
            {
                UE_LOG(LogTemp, Error, TEXT("PatrolRouteActor %s has no SplineComponent!"), *PatrolRouteActor->GetName());
            }
        }

    }

    float ABaseEnemyCharacter::TakeDamage(float DamageCount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
    {
        float Applied = Super::TakeDamage(DamageCount, DamageEvent, EventInstigator, DamageCauser);

        if (CharacterStats.MaxHP > 0.f)
        {
            float NewFrac = FMath::Clamp(CharacterStats.HP / CharacterStats.MaxHP, 0.f, 1.f);

            if (IsValid(EnemyHPWidgetInstance))
            {
                EnemyHPWidgetInstance->SetHPFraction(NewFrac);
            }
            else if (IsValid(EnemyRawWidgetInstance))
            {
                // reflection fallback
                FName FuncName = TEXT("SetHPFraction");
                UFunction* Func = EnemyRawWidgetInstance->FindFunction(FuncName);
                if (Func)
                {
                    struct FParam { float NewFraction; };
                    FParam P{ NewFrac };
                    EnemyRawWidgetInstance->ProcessEvent(Func, &P);
                    UE_LOG(LogTemp, Log, TEXT("[%s] Called SetHPFraction via ProcessEvent on raw widget"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[%s] Raw widget has no SetHPFraction function (class: %s)"),
                        *GetName(), *GetNameSafe(EnemyRawWidgetInstance->GetClass()));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] No widget instance to update."), *GetName());
            }
        }


        return Applied;
    }

    void ABaseEnemyCharacter::SpawnVerticalBeamAtActor(AActor* TargetActor, float Duration, bool bAttachToTarget, float BeamScale)
    {
        if (!TargetActor || !VerticalBeamSpellClass) return;

        UWorld* World = GetWorld();
        if (!World) return;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.Owner = this;

        FVector SpawnLoc = TargetActor->GetActorLocation();
        FRotator SpawnRot = FRotator::ZeroRotator;

        AVerticalBeamSpell* Beam = World->SpawnActor<AVerticalBeamSpell>(VerticalBeamSpellClass, SpawnLoc, SpawnRot, Params);
        if (!Beam) return;

        // Optionally assign the Niagara asset on the spawned actor if you want to set it dynamically:
        // Beam->BeamFX = VerticalBeamFX; // if you kept VerticalBeamFX as member on enemy

        Beam->Initialize(TargetActor, bAttachToTarget, Duration, BeamScale, /*DamagePerSecond=*/20.f, /*Tick=*/0.2f);
    }

    void ABaseEnemyCharacter::Tick(float DeltaTime)
    {
        Super::Tick(DeltaTime);

        // 2) Smooth yaw‐only on the character
        FRotator Current = GetActorRotation();
        FRotator TargetYawOnly(0, DesiredAimRotation.Yaw, 0);
        FRotator Smooth = FMath::RInterpTo(Current, TargetYawOnly, DeltaTime, 10.0f);
        SetActorRotation(Smooth);

        // Face toward local player's camera (if present)
        if (HealthWidgetComponent && HealthWidgetComponent->GetWidget() && GetWorld())
        {
            APlayerController* PC = GetWorld()->GetFirstPlayerController();
            if (PC && PC->PlayerCameraManager)
            {
                FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
                FVector WidgetLoc = HealthWidgetComponent->GetComponentLocation();
                // Build a rotation that faces the camera but stays upright
                FRotator LookAt = (CamLoc - WidgetLoc).Rotation();
                LookAt.Pitch = 0.f; // keep horizontal
                LookAt.Roll = 0.f;
                HealthWidgetComponent->SetWorldRotation(LookAt);
            }
        }

        if (!PatrolRouteActor) return;

        // 1) Ask the route for its current target
        const FVector CurrentLocation = GetActorLocation();
        const FVector TargetLocation = PatrolRouteActor->GetWorldSplineLocation();
        const float   Radius = PatrolRouteActor->GetAcceptanceRadius();

        // 2) Compute move-vector toward it
        const FVector MoveDir = (TargetLocation - CurrentLocation).GetSafeNormal();

        if (!MoveDir.IsNearlyZero())
        {
            FRotator DesiredRot = MoveDir.Rotation();
            FRotator Smoothed = FMath::RInterpTo(GetActorRotation(), DesiredRot, DeltaTime, 10.f);
            SetActorRotation(Smoothed);
        }

    }