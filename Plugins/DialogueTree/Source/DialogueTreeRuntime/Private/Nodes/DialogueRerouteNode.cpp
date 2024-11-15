// Copyright Zachary Brett, 2024. All rights reserved.

//Header
#include "Nodes/DialogueRerouteNode.h"
//Plugin
#include "Dialogue.h"
#include "LogDialogueTree.h"

void UDialogueRerouteNode::EnterNode()
{
	check(Dialogue);

	//Call super
	Super::EnterNode();

	//If no children, end dialogue and throw error
	if (Children.Num() < 1 || Children[0] == nullptr)
	{
		UE_LOG(
			LogDialogueTree,
			Warning,
			TEXT("Exiting dialogue: Entered a reroute node with no children...")
		);
		Dialogue->EndDialogue();
		return;
	}

	//Otherwise, get first (only) child and enter that node 
	Dialogue->TraverseNode(Children[0]);
}

FDialogueOption UDialogueRerouteNode::GetAsOption()
{
	if (Children.IsEmpty() || Children[0] == nullptr)
	{
		return FDialogueOption();
	}

	return Children[0]->GetAsOption();
}
