// Copyright Zachary Brett, 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/DialogueNode.h"
#include "DialogueOptionLockNode.generated.h"

class UDialogueCondition;

/**
 * Dialogue node that serves as a conditional filter for a single option. 
 * If the condition fails to pass, the option is marked as "locked."
 */
UCLASS()
class DIALOGUETREERUNTIME_API UDialogueOptionLockNode : public UDialogueNode
{
	GENERATED_BODY()
	
public:
	/** UDialogueNode Implementation */
	virtual FDialogueOption GetAsOption() override;
	virtual void EnterNode() override;
	/** End UDialogueNode */

public:
	/**
	* Populates the basic data for the lock node.
	*
	* @param InIfAny - bool, whether the conditions are linked together via an
	* "or" relationship versus an "and" relationship.
	* @param InConditions - TArray<UDialogueCondition*>&, array of conditions.
	* @param LockedText - FText - optional message when locked
	* @param UnlockedText - FText - optional message when unlocked
	*/
	void InitLockNodeData(bool InIfAny, 
		TArray<UDialogueCondition*>& InConditions, 
		const FText& LockedText, const FText& UnlockedText);

	/**
	* Get the if any value of the node.
	*
	* @return bool, true if any condition can be true for the node as a whole
	* to be true.
	*/
	bool GetIfAny() const;

	/**
	* Get the message to use when the node is locked.
	*
	* @return FText, the locked message.
	*/
	FText GetLockedMessage() const;

	/**
	* Get the message to use when the node is unlocked.
	*
	* @return FText, the unlocked message.
	*/
	FText GetUnlockedMessage() const;

	/**
	* Get the conditions that are part of this lock node.
	*
	* @return TArray<TObjectPtr<UDialogueCondition>>&, the conditions.
	*/
	const TArray<TObjectPtr<UDialogueCondition>>& GetConditions() const;

private:
	/**
	* Determines if the branch node passes its conditions to
	* transition to the "true" node.
	*
	* @return bool - true if conditions are passed, false otherwise.
	*/
	bool PassesConditions() const;

	/**
	* Determines if any condition in the conditions list is true.
	*
	* @return bool - true if any condition is true, else false.
	*/
	bool AnyConditionsTrue() const;

	/**
	* Determines if all conditions in the conditions list are true.
	*
	* @return bool - true if all conditions are true, else false.
	*/
	bool AllConditionsTrue() const;

private:
	/** Conditions which govern branching */
	UPROPERTY()
	TArray<TObjectPtr<UDialogueCondition>> Conditions;

	/** If the branch should evaluate true if any condition does */
	UPROPERTY()
	bool bIfAny = false;

	/** Optional message to include when locked */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText LockedMessage = FText();

	/** Optional message to include when unlocked */
	FText UnlockedMessage = FText();
};
