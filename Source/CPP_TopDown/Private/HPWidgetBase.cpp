// Fill out your copyright notice in the Description page of Project Settings.


#include "HPWidgetBase.h"

void UHPWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    // Create dynamic material instance from the image brush material if available
    if (HPBarImage)
    {
        UMaterialInterface* Mat = Cast<UMaterialInterface>(HPBarImage->GetBrush().GetResourceObject());
        if (Mat)
        {
            BarMID = UMaterialInstanceDynamic::Create(Mat, this);

            if (BarMID)
            {
                // Assign the dynamic instance to the Image via UImage helper
                HPBarImage->SetBrushFromMaterial(BarMID);

                // Initialize parameters
                BarMID->SetScalarParameterValue(TEXT("Current_HP"), CurrentHP);
                BarMID->SetScalarParameterValue(TEXT("Lost_HP"), LostHP);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UPlayer_HUD::NativeConstruct: HPBarImage brush has no material resource."));
        }
    }
}

void UHPWidgetBase::SetHPFraction(float NewFraction)
{
    NewFraction = FMath::Clamp(NewFraction, 0.f, 1.f);

    // Immediately update the actual current fill
    CurrentHP = NewFraction;
    if (BarMID)
    {
        BarMID->SetScalarParameterValue(TEXT("Current_HP"), CurrentHP);
    }

    if (!bAnimatePrev)
    {
        LostHP = CurrentHP;
        if (BarMID)
            BarMID->SetScalarParameterValue(TEXT("Lost_HP"), LostHP);
        return;
    }

    // Start animation: PrevFill currently holds previous value, we animate it toward CurrentFill
    Elapsed = 0.f;
    StartTickAnimation();
}

void UHPWidgetBase::StartTickAnimation()
{
    StopTickAnimation(); // ensure single timer

    if (TickDuration <= 0.f)
    {
        // instant
        LostHP = CurrentHP;
        if (BarMID) BarMID->SetScalarParameterValue(TEXT("Lost_HP"), LostHP);
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UHPWidgetBase::TickAnimationStep, 1.f / 60.f, true);
}

void UHPWidgetBase::StopTickAnimation()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TickHandle);
    }
}

void UHPWidgetBase::TickAnimationStep()
{
    if (!GetWorld()) return;

    Elapsed += (1.f / 60.f); // time step matched to timer frequency
    float Alpha = FMath::Clamp(Elapsed / TickDuration, 0.f, 1.f);

    // Smooth interpolation (ease) — tweak Easing as you like
    float NewPrev = FMath::Lerp(LostHP, CurrentHP, Alpha); // linear; can use ease: FMath::InterpEaseOut

    LostHP = NewPrev;
    if (BarMID)
    {
        BarMID->SetScalarParameterValue(TEXT("Lost_HP"), LostHP);
    }

    // stop when close enough or finished
    if (Alpha >= 1.f || FMath::IsNearlyEqual(LostHP, CurrentHP, 0.001f))
    {
        LostHP = CurrentHP;
        if (BarMID) BarMID->SetScalarParameterValue(TEXT("Lost_HP"), LostHP);
        StopTickAnimation();
    }
}

