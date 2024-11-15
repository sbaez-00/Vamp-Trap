// Fill out your copyright notice in the Description page of Project Settings.


#include "NotRecentWorldCondition.h"

#include "WorldConditions/SmartObjectWorldConditionBase.h"
#include "WorldConditions/Public/WorldConditionBase.h"
#include "WorldConditions/SmartObjectWorldConditionSchema.h"
#include "WorldConditionSchema.h"
#include "WorldConditionContext.h"
#include "WorldConditionTypes.h"
#include "SmartObjectComponent.h"


#include "ANpcBase.h"



//.cpp
bool FNotRecentWorldCondition::Initialize(const UWorldConditionSchema& Schema)
{
	//Initialize is used to collect data references from the schema
	//so they can be used later
	const USmartObjectWorldConditionSchema* SmartObjectSchema = Cast<USmartObjectWorldConditionSchema>(&Schema);

	SmartObjectActorRef = SmartObjectSchema->GetSmartObjectActorRef();
	UserActorRef = SmartObjectSchema->GetUserActorRef();

	return true;
}

bool FNotRecentWorldCondition::Activate(const FWorldConditionContext& Context) const
{
	//Activate can be used to validate that the references are valid, or
	//perform other kinds of initialization work
	if (!SmartObjectActorRef.IsValid())
	{
		return false;
	}

	if (!UserActorRef.IsValid())
	{
		return false;
	}

	return true;
}

FWorldConditionResult FNotRecentWorldCondition::IsTrue(const FWorldConditionContext& Context) const
{
	//Perform the actual condition checking here

	//We can get the actual objects the references point to here:
	const AActor* const SmartObjectActor = Context.GetContextDataPtr<AActor>(SmartObjectActorRef);
	const AActor* const UserActor = Context.GetContextDataPtr<AActor>(UserActorRef);

	FWorldConditionResult Result(EWorldConditionResultValue::IsFalse, false);

	const AANpcBase* npc = Cast<AANpcBase>(UserActor);

	if (npc == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("null npc"));
		return Result;
	}

	const USmartObjectComponent* SmartObjectComponent = SmartObjectActor->GetComponentByClass<USmartObjectComponent>();

	if (npc->PreviousSmartObjectSlotHandle.GetSmartObjectHandle() != SmartObjectComponent->GetRegisteredHandle()) {
		Result.Value = EWorldConditionResultValue::IsTrue;
	}

	return Result;
}

#if WITH_EDITOR
FText FNotRecentWorldCondition::GetDescription() const
{
	return INVTEXT("Nice Description for Editor");
}
#endif