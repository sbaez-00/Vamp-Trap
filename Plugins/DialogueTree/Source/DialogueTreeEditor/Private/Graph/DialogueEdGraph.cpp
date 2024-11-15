// Copyright Zachary Brett, 2024. All rights reserved.

//Header
#include "Graph/DialogueEdGraph.h"
//UE
#include "GraphEditAction.h"
#include "Settings/EditorStyleSettings.h"
//Plugin
#include "Dialogue.h"
#include "DialogueSpeakerSocket.h"
#include "Graph/Nodes/GraphNodeDialogue.h"
#include "Graph/Nodes/GraphNodeDialogueBranch.h"
#include "Graph/Nodes/GraphNodeDialogueEntry.h"
#include "Graph/Nodes/GraphNodeDialogueEntry.h"
#include "Graph/Nodes/GraphNodeDialogueEvent.h"
#include "Graph/Nodes/GraphNodeDialogueJump.h"
#include "Graph/Nodes/GraphNodeDialogueOptionLock.h"
#include "Graph/Nodes/GraphNodeDialogueReroute.h"
#include "Graph/Nodes/GraphNodeDialogueSpeech.h"
#include "Nodes/DialogueNode.h"
#include "Nodes/DialogueBranchNode.h"
#include "Nodes/DialogueEntryNode.h"
#include "Nodes/DialogueEventNode.h"
#include "Nodes/DialogueJumpNode.h"
#include "Nodes/DialogueOptionLockNode.h"
#include "Nodes/DialogueRerouteNode.h"
#include "Nodes/DialogueSpeechNode.h"

UDialogueEdGraph::UDialogueEdGraph()
{
	//Setup handler for changing the graph
	AddOnGraphChangedHandler(
		FOnGraphChanged::FDelegate::CreateUObject(
			this,
			&UDialogueEdGraph::OnDialogueGraphChanged
		)
	);
}

bool UDialogueEdGraph::Modify(bool bAlwaysMarkDirty)
{
	bool ModifyReturnValue = Super::Modify(bAlwaysMarkDirty);
	GetDialogue()->Modify();

	for (UEdGraphNode* Node : Nodes)
	{
		Node->Modify();
	}

	return ModifyReturnValue;
}

void UDialogueEdGraph::PostEditUndo()
{
	Super::PostEditUndo();
	NotifyGraphChanged();
}

void UDialogueEdGraph::PostInitProperties()
{
	Super::PostInitProperties();

	//Set up speaker roles changed event
	UDialogue* OuterDialogue = Cast<UDialogue>(GetOuter());
	if (OuterDialogue)
	{
		OuterDialogue->OnSpeakerRolesChanged.BindUFunction(
			this,
			"OnSpeakerRolesChanged"
		);
	}
}

UDialogue* UDialogueEdGraph::GetDialogue() const
{
	return CastChecked<UDialogue>(GetOuter());
}

void UDialogueEdGraph::AddToNodeMap(UGraphNodeDialogue* InNode)
{
	check(InNode);
	NodeMap.Add(InNode->GetID(), InNode);
}

void UDialogueEdGraph::RemoveFromNodeMap(FName RemoveID)
{
	NodeMap.Remove(RemoveID);
}

bool UDialogueEdGraph::ContainsNode(FName InID) const
{
	return NodeMap.Contains(InID);
}

void UDialogueEdGraph::SetGraphRoot(UGraphNodeDialogue* InRoot)
{
	Root = InRoot;
}

UGraphNodeDialogue* UDialogueEdGraph::GetNode(FName InID) const
{
	if (NodeMap.Contains(InID))
	{
		return NodeMap[InID];
	}

	return nullptr;
}

TArray<UGraphNodeDialogue*> UDialogueEdGraph::GetAllNodes() const
{
	TArray<UGraphNodeDialogue*> DialogueNodes;
	for (auto& Entry : NodeMap)
	{
		if (Entry.Value)
		{
			DialogueNodes.Add(Entry.Value);
		}
	}

	return DialogueNodes;
}

bool UDialogueEdGraph::HasSpeaker(FName InName) const
{
	return GetDialogue()->GetSpeakerRoles().Contains(InName);
}

FColor UDialogueEdGraph::GetSpeakerColor(FName InName) const
{
	if (GetDialogue()->GetSpeakerRoles().Contains(InName))
	{
		return GetDialogue()->GetSpeakerRoles()[InName].GraphColor;
	}

	return FColor::White;
}

TArray<UDialogueSpeakerSocket*> UDialogueEdGraph::GetAllSpeakers() const
{
	TArray<UDialogueSpeakerSocket*> AllSpeakers;
	for (auto& Entry : GetDialogue()->GetSpeakerRoles())
	{
		AllSpeakers.Add(Entry.Value.SpeakerSocket);
	}
	return AllSpeakers;
}

void UDialogueEdGraph::CompileAsset()
{
	//Verify asset and root exist 
	UDialogue* Asset = GetDialogue();
	check(Asset && Root);

	//Prepare the dialogue to be compiled
	Asset->PreCompileDialogue();

	//Clear asset nodes
	ClearAssetNodes();

	//Compile asset tree
	CreateAssetNodes(Asset);
	Asset->SetRootNode(Root->GetAssetNode());

	TSet<UGraphNodeDialogue*> VisitedNodes;
	UpdateAssetTreeRecursive(Root, VisitedNodes);
	FinalizeAssetNodes();

	//Determine if compilation was successful
	if (CanCompileAsset())
	{
		Asset->SetCompileStatus(EDialogueCompileStatus::Compiled);
	}
	else
	{
		Asset->SetCompileStatus(EDialogueCompileStatus::Failed);
	}
}

bool UDialogueEdGraph::CanCompileAsset() const
{
	//Get all nodes
	TArray<UGraphNodeDialogue*> DialogueNodes;
	GetNodesOfClass<UGraphNodeDialogue>(DialogueNodes);

	//Verify all nodes can compile
	bool bCanCompile = true;
	for (UGraphNodeDialogue* Node : DialogueNodes)
	{
		if (!Node->CanCompileNode())
		{
			bCanCompile = false;
		}
	}

	return bCanCompile;
}

bool UDialogueEdGraph::TryBuildGraphFromAsset(const UDialogue* InAsset)
{
	//If the asset is blank, we cannot create one
	if (!InAsset || !InAsset->HasExistingData())
	{
		return false;
	}

	//If this graph is already generated, we are done
	if (!Nodes.IsEmpty() && InAsset->GetEdGraph() == this)
	{
		return true;
	}

	AddNodesFromAsset(InAsset);
	
	UGraphNodeDialogueEntry* EntryNode = CastChecked<UGraphNodeDialogueEntry>(
		GetNode(InAsset->GetRootNode()->GetNodeID())
	);
	SetGraphRoot(EntryNode);

	RegenerateNodeLinks();

	return true; 
}

void UDialogueEdGraph::AddNodesFromAsset(const UDialogue* InAsset)
{
	check(InAsset);

	Nodes.Empty();
	NodeMap.Empty();

	const TArray<UDialogueNode*> AssetNodes = InAsset->GetAllNodes();

	for (UDialogueNode* AssetNode : AssetNodes)
	{
		UGraphNodeDialogue* NewNode = CreateGraphNodeFromAssetNode(AssetNode);
		AddNode(NewNode);
		AddToNodeMap(NewNode);
	}
}

void UDialogueEdGraph::RegenerateNodeLinks()
{
	for (auto& Pair : NodeMap)
	{
		UGraphNodeDialogue* Node = Pair.Value.Get();
		Node->RegenerateNodeConnections(this);
	}
}

UGraphNodeDialogue* UDialogueEdGraph::CreateGraphNodeFromAssetNode(
	UDialogueNode* AssetNode
)
{
	if (!AssetNode)
	{
		return nullptr;
	}

	/**
	* Todo: Speed is relevant so this will work for now. Eventually I want to 
	* move this into a dedicated factory, both for clarity's sake and to 
	* decouple the graph from its individual nodes. 
	*/
	UGraphNodeDialogue* NewNode = nullptr;
	if (UDialogueSpeechNode* SpeechNode 
		= Cast<UDialogueSpeechNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueSpeech>(this);
	}

	if (UDialogueBranchNode* BranchNode
		= Cast<UDialogueBranchNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueBranch>(this);
	}

	if (UDialogueEventNode* EventNode
		= Cast<UDialogueEventNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueEvent>(this);
	}

	if (UDialogueEntryNode* EntryNode
		= Cast<UDialogueEntryNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueEntry>(this);
	}

	if (UDialogueJumpNode* JumpNode =
		Cast <UDialogueJumpNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueJump>(this);
	}

	if (UDialogueOptionLockNode* OptionLockNode =
		Cast<UDialogueOptionLockNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueOptionLock>(this);
	}

	if (UDialogueRerouteNode* RerouteNode =
		Cast<UDialogueRerouteNode>(AssetNode))
	{
		NewNode = NewObject<UGraphNodeDialogueReroute>(this);
	}
	NewNode->CreateNewGuid();
	NewNode->AllocateDefaultPins();
	NewNode->LoadNodeData(AssetNode);
	NewNode->SnapToGrid(GetDefault<UEditorStyleSettings>()->GridSnapSize);

	return NewNode;
}

UDialogueSpeakerSocket* UDialogueEdGraph::GetSpeakerSocketFromName(
	FName InName
) const
{
	const TMap<FName, FSpeakerField>& SpeakerRoles =
		GetDialogue()->GetSpeakerRoles();
	if (SpeakerRoles.Contains(InName))
	{
		const FSpeakerField& TargetSpeaker = SpeakerRoles[InName];
		return TargetSpeaker.SpeakerSocket;
	}

	return nullptr;
}

void UDialogueEdGraph::UpdateAllNodeVisuals()
{
	for (auto& Entry : NodeMap)
	{
		Entry.Value->UpdateDialogueNode();
	}
}

void UDialogueEdGraph::ClearAssetNodes()
{
	for (UEdGraphNode* Current : Nodes)
	{
		if (UGraphNodeDialogue* DialogueNode =
			Cast<UGraphNodeDialogue>(Current))
		{
			DialogueNode->ClearAssetNode();
		}
	}
}

void UDialogueEdGraph::CreateAssetNodes(UDialogue* InAsset)
{
	for (auto& Entry : NodeMap)
	{
		check(Entry.Value);
		Entry.Value->CreateAssetNode(InAsset);
		Entry.Value->AssignAssetNodeID();
		Entry.Value->AssignAssetNodeCommonData();
		InAsset->AddNode(Entry.Value->GetAssetNode());
	}
}

void UDialogueEdGraph::FinalizeAssetNodes()
{
	for (auto& Entry : NodeMap)
	{
		check(Entry.Value);
		Entry.Value->FinalizeAssetNode();
	}
}

void UDialogueEdGraph::UpdateAssetTreeRecursive(UGraphNodeDialogue* InRoot,
	TSet<UGraphNodeDialogue*> VisitedNodes)
{
	check(InRoot);

	if (VisitedNodes.Contains(InRoot))
	{
		return;
	}
	else
	{
		VisitedNodes.Add(InRoot);
	}

	//Link the asset root to its parents
	InRoot->LinkAssetNode();

	//Retrieve children and order left to right
	TArray<UGraphNodeDialogue*> OutChildren;
	InRoot->GetChildren(OutChildren);
	UGraphNodeDialogue::SortNodesLeftToRight(OutChildren);

	//Recur over children
	for (UGraphNodeDialogue* Child : OutChildren)
	{
		UpdateAssetTreeRecursive(Child, VisitedNodes);
	}
}

void UDialogueEdGraph::OnDialogueGraphChanged(
	const FEdGraphEditAction& EditAction)
{
	//If removing a node, pull that node from the node map
	if (EditAction.Action == GRAPHACTION_RemoveNode)
	{ 
		for (const UEdGraphNode* Removed : EditAction.Nodes)
		{
			const UGraphNodeDialogue* RemovedNode =
				Cast<UGraphNodeDialogue>(Removed);

			if (RemovedNode)
			{
				NodeMap.Remove(RemovedNode->GetID());
			}
		}
	}
}

void UDialogueEdGraph::OnSpeakerRolesChanged()
{
	CanCompileAsset(); //Check for error banners
	UpdateAllNodeVisuals();
}
