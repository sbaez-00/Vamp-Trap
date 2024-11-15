// Copyright Zachary Brett, 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/DialogueNode.h"
#include "DialogueRerouteNode.generated.h"

/**
 * Runtime placeholder node for reroute nodes in the graph. 
 */
UCLASS()
class DIALOGUETREERUNTIME_API UDialogueRerouteNode : public UDialogueNode
{
	GENERATED_BODY()
	
public:
	/** UDialogueNode Impl. */
	virtual void EnterNode() override;
	virtual FDialogueOption GetAsOption() override;
	/** End UDialogueNode */
};
