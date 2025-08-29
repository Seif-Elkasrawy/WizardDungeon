// Fill out your copyright notice in the Description page of Project Settings.

#include "Player_HUD.h"
#include "HPWidgetBase.h"

void UPlayer_HUD::SetPlayerHPFraction(float NewFraction)
{
    if (HPWidget)
    {
        HPWidget->SetHPFraction(FMath::Clamp(NewFraction, 0.0f, 1.0f)); // Fixed parameter name
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPlayer_HUD::SetPlayerHPFraction: HPWidget is null"));
    }
}
