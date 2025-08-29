// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player_HUD.generated.h"


class UHPWidgetBase;
/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UPlayer_HUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
    /** Update target HP fraction (0..1). Call this when HP changes. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetPlayerHPFraction(float NewFraction);

protected:
    // Bind this in the UMG designer: name the widget exactly "HPWidget" and enable "Is Variable"
    UPROPERTY(meta = (BindWidgetOptional))
    UHPWidgetBase* HPWidget = nullptr;
};
