// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WorldConditions/SmartObjectWorldConditionBase.h"
#include "WorldConditions/SmartObjectWorldConditionSchema.h"
#include "WorldConditions/Public/WorldConditionQuery.h"
#include "WorldConditionSchema.h"
#include "WorldConditionContext.h"
#include "WorldConditionTypes.h"



#include "NotRecentWorldCondition.generated.h"

/**
 * 
 */
USTRUCT(DisplayName = "Not recent condition")
struct VAMP_TRAP_API FNotRecentWorldCondition : public FSmartObjectWorldConditionBase
{
	GENERATED_BODY()


	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual bool Activate(const FWorldConditionContext& Context) const override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;

	FWorldConditionContextDataRef SmartObjectActorRef;
	FWorldConditionContextDataRef UserActorRef;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif
};
