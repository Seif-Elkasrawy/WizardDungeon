// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_AttackTarget.generated.h"

/**
 * 
 */
UCLASS()
class CPP_TOPDOWN_API UEnvQueryContext_AttackTarget : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:

	virtual void ProvideSingleActor(UObject* QuerierObject, AActor*& ResultingActor) const;

};
