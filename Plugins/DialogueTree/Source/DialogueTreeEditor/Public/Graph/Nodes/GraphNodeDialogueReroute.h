// Copyright Zachary Brett, 2024. All rights reserved.

#pragma once

//UE
#include "CoreMinimal.h"
//Plugin
#include "GraphNodeDialogue.h"
//Generated
#include "GraphNodeDialogueReroute.generated.h"

/**
 * Node that redirects dialogue flow between nodes with actual content. 
 */
UCLASS()
class DIALOGUETREEEDITOR_API UGraphNodeDialogueReroute :
	public UGraphNodeDialogue
{
	GENERATED_BODY()
	
public:
	/** UEdGraphNode Impl. */
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex,
		int32& OutOutputPinIndex) const override;
	virtual void AllocateDefaultPins() override;
	/** End UEdGraphNode */

	/** UGraphNodeDialogue Impl. */
	virtual void CreateAssetNode(class UDialogue* InAsset) override;
	virtual FName GetBaseID() const override;
	/** End UEdGraphNode */
};
