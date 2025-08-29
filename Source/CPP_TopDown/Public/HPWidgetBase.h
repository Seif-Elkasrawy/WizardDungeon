// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HPWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UHPWidgetBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

    /** Update target HP fraction (0..1). Call this when HP changes. */
    UFUNCTION(BlueprintCallable)
    void SetHPFraction(float NewFraction);

    /** How long the tick bar takes to lerp down (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP")
    float TickDuration = 0.6f;

    /** If true, apply the change immediately to CurrentFill then animate PrevFill down */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP")
    bool bAnimatePrev = true;

protected:
    /** Bind this in the UMG designer (Image widget that uses the material) */
    UPROPERTY(meta = (BindWidget))
    UImage* HPBarImage;

    // dynamic material instance we drive
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* BarMID = nullptr;

    // internal tracking
    float CurrentHP = 1.0f; // actual HP percent
    float LostHP = 1.0f;    // shown "previous" bar that slides toward CurrentFill
    float Elapsed = 0.0f;

    FTimerHandle TickHandle;

    void StartTickAnimation();
    void StopTickAnimation();

    void TickAnimationStep();
};
