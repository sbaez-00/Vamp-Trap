// Copyright Zachary Brett, 2024. All rights reserved.

//Header
#include "Graph/Nodes/GraphNodeDialogueReroute.h"
//Plugin
#include "Dialogue.h"
#include "Nodes/DialogueRerouteNode.h"

bool UGraphNodeDialogueReroute::ShouldDrawNodeAsControlPointOnly(
	int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}

void UGraphNodeDialogueReroute::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName());
	CreatePin(EGPD_Output, "MultipleNodes", FName());
}

void UGraphNodeDialogueReroute::CreateAssetNode(UDialogue* InAsset)
{
	UDialogueRerouteNode* NewNode =
		NewObject<UDialogueRerouteNode>(InAsset);

	SetAssetNode(NewNode);
}

FName UGraphNodeDialogueReroute::GetBaseID() const
{
	return FName("Reroute");
}


