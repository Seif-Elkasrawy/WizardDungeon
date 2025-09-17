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

        // --- Init the health widget (hardened & verbose) ---
        UE_LOG(LogTemp, Log, TEXT("%s::BeginPlay() - starting widget init"), *GetName());

        // 1) Ensure we have a valid HealthWidgetComponent pointer - try fallbacks if not
        if (!IsValid(HealthWidgetComponent))
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] HealthWidgetComponent pointer invalid or null. Attempting recovery..."), *GetName());

            // Fallback #1: find first widget component on the actor
            UWidgetComponent* Found = FindComponentByClass<UWidgetComponent>();
            if (Found)
            {
                HealthWidgetComponent = Found;
                UE_LOG(LogTemp, Log, TEXT("[%s] Found UWidgetComponent via FindComponentByClass: %s"), *GetName(), *Found->GetName());
            }
            else
            {
                // Fallback #2: search by name (in case BP created a component named "HealthWidget")
                TArray<UWidgetComponent*> WidgetComps;
                GetComponents<UWidgetComponent>(WidgetComps); // fills array for you

                for (UWidgetComponent* WC : WidgetComps)
                {
                    if (!IsValid(WC)) continue;
                    if (WC->GetName().Contains(TEXT("HealthWidget")))
                    {
                        HealthWidgetComponent = WC;
                        UE_LOG(LogTemp, Log, TEXT("[%s] Found UWidgetComponent via templated GetComponents: %s"), *GetName(), *WC->GetName());
                        break;
                    }
                }

            }
        }

        // If still invalid, give up gracefully (do not call into UMG)
        if (!IsValid(HealthWidgetComponent))
        {
            UE_LOG(LogTemp, Error, TEXT("[%s] No HealthWidgetComponent available! Skipping widget init."), *GetName());
            // don't return here if you still want other initialization to continue
        }
        else
        {
            // Ensure component registered
            if (!HealthWidgetComponent->IsRegistered())
            {
                HealthWidgetComponent->RegisterComponent();
                UE_LOG(LogTemp, Log, TEXT("[%s] Registered HealthWidgetComponent."), *GetName());
            }

            // Make sure we have a widget class to create
            TSubclassOf<UUserWidget> ChosenClass = EnemyHealthWidgetClass ? EnemyHealthWidgetClass : DefaultEnemyHealthWidgetClass;
            if (ChosenClass == nullptr)
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] EnemyHealthWidgetClass and DefaultEnemyHealthWidgetClass are null; no widget will be created."), *GetName());
            }
            else
            {
                HealthWidgetComponent->SetWidgetClass(ChosenClass);

                UUserWidget* RawWidget = HealthWidgetComponent->GetUserWidgetObject();
                if (!RawWidget)
                {
                    // InitWidget will create the widget instance if not present.
                    HealthWidgetComponent->InitWidget();
                    RawWidget = HealthWidgetComponent->GetUserWidgetObject();
                }

                UE_LOG(LogTemp, Log, TEXT("[%s] Raw widget created: %s (class: %s)"),
                    *GetName(),
                    *GetNameSafe(RawWidget),
                    *GetNameSafe(RawWidget ? RawWidget->GetClass() : nullptr));

                EnemyRawWidgetInstance = RawWidget;
                EnemyHPWidgetInstance = RawWidget ? Cast<UHPWidgetBase>(RawWidget) : nullptr;

                if (EnemyHPWidgetInstance)
                {
                    float InitFrac = (CharacterStats.MaxHP > 0.f) ? (CharacterStats.HP / CharacterStats.MaxHP) : 0.f;
                    EnemyHPWidgetInstance->SetHPFraction(InitFrac);
                    UE_LOG(LogTemp, Log, TEXT("[%s] EnemyHPWidgetInstance initialized."), *GetName());
                }
                else if (EnemyRawWidgetInstance)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[%s] Raw widget exists but cast to UHPWidgetBase failed (class=%s). Will use ProcessEvent fallback."),
                        *GetName(), *GetNameSafe(EnemyRawWidgetInstance->GetClass()));
                    // Optional: call SetHPFraction once via ProcessEvent here if needed
                }
            }
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

    void ABaseEnemyCharacter::SpawnVerticalBeamAtActor(AActor* TargetActor, float Duration, float BeamScale)
    {
        if (!TargetActor || !VerticalBeamSpellClass) return;

        UWorld* World = GetWorld();
        if (!World) return;

        AVerticalBeamSpell* Beam = nullptr;

        if (BaseSpellPool) {
			APooledActor* PooledActor = BaseSpellPool->GetPooledActor(VerticalBeamSpellClass);
			Beam = Cast<AVerticalBeamSpell>(PooledActor);
            if (Beam) {
                Beam->SetActorLocation(TargetActor->GetActorLocation());
                Beam->Initialize(TargetActor, Duration, BeamScale, /*DamagePerSecond=*/20.f, /*Tick=*/0.2f);
                return;
            }
            else {
				UE_LOG(LogTemp, Error, TEXT("[%s] SpawnVerticalBeamAtActor: Pooled actor is not a AVerticalBeamSpell!"), *GetName());
            }
        }
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
        if (IsValid(HealthWidgetComponent) && HealthWidgetComponent->GetWidget() && GetWorld())
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