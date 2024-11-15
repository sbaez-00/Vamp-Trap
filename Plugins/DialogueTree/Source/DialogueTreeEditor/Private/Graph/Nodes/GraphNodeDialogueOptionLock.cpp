// Copyright Zachary Brett, 2024. All rights reserved.

//Header
#include "Graph/Nodes/GraphNodeDialogueOptionLock.h"
//Plugin
#include "Conditionals/DialogueCondition.h"
#include "Conditionals/Queries/Base/DialogueQuery.h"
#include "Dialogue.h"
#include "Graph/DialogueEdGraph.h"
#include "Graph/DialogueGraphCondition.h"
#include "Nodes/DialogueOptionLockNode.h"
//UE
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "GraphNodeDialogueOptionLock"

UGraphNodeDialogueOptionLock* UGraphNodeDialogueOptionLock::MakeTemplate(UObject* Outer)
{
    return NewObject<UGraphNodeDialogueOptionLock>(Outer);
}

void UGraphNodeDialogueOptionLock::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, "MultipleNodes", FName());
    CreatePin(EGPD_Output, "MultipleNodes", FName());
}

FText UGraphNodeDialogueOptionLock::GetTooltipText() const
{
    return LOCTEXT(
        "NodeTooltip",
        "Allows the user to mark a dialogue child option as 'locked' unless a condition is passed."
    );
}

FName UGraphNodeDialogueOptionLock::GetBaseID() const
{
    return FName("Option Lock");
}

void UGraphNodeDialogueOptionLock::PostPasteNode()
{
    Super::PostPasteNode();

    //Copy conditions so they are no longer shared with copied node
    TArray<UDialogueGraphCondition*> CopiedConditions;
    for (UDialogueGraphCondition* OldCondition : Conditions)
    {
        UDialogueGraphCondition* NewCondition =
            DuplicateObject(OldCondition, this);
        if (NewCondition)
        {
            CopiedConditions.Add(NewCondition);
        }
    }

    //Replace old conditions with new ones
    Conditions.Empty();
    for (UDialogueGraphCondition* NewCondition : CopiedConditions)
    {
        Conditions.Add(NewCondition);
    }
}

void UGraphNodeDialogueOptionLock::CreateAssetNode(UDialogue* InAsset)
{
    UDialogueOptionLockNode* NewNode =
        NewObject<UDialogueOptionLockNode>(InAsset);

    SetAssetNode(NewNode);
}

void UGraphNodeDialogueOptionLock::FinalizeAssetNode()
{
    //Get asset node
    UDialogueOptionLockNode* TargetLockNode =
        CastChecked<UDialogueOptionLockNode>(GetAssetNode());

    //Verify we have a dialogue
    check(GetDialogueGraph());
    UDialogue* TargetDialogue = GetDialogueGraph()->GetDialogue();
    check(TargetDialogue);

    //Finalize and get conditions
    TArray<UDialogueCondition*> AssetConditions;

    for (UDialogueGraphCondition* GraphCondition : Conditions)
    {
        GraphCondition->FinalizeCondition(TargetDialogue);
        UDialogueCondition* Condition = DuplicateObject<UDialogueCondition>(
            GraphCondition->GetCondition(),
            TargetDialogue
        );

        if (Condition)
        {
            AssetConditions.Add(Condition);
        }
    }

    TargetLockNode->InitLockNodeData(
        bIfAny, 
        AssetConditions, 
        LockedMessage,
        UnlockedMessage
    );
}

bool UGraphNodeDialogueOptionLock::CanCompileNode()
{
    for (UDialogueGraphCondition* GraphCondition : Conditions)
    {
        UDialogueCondition* Condition = GraphCondition->GetCondition();

        if (!Condition || !Condition->IsValidCondition())
        {
            SetErrorFlag(true);
            return false;
        }
    }

    SetErrorFlag(false);
    return true;
}

void UGraphNodeDialogueOptionLock::LoadNodeData(UDialogueNode* InNode)
{
    Super::LoadNodeData(InNode);

    UDialogueOptionLockNode* LockNode =
        CastChecked<UDialogueOptionLockNode>(InNode);
    
    bIfAny = LockNode->GetIfAny();
    LockedMessage = LockNode->GetLockedMessage();
    UnlockedMessage = LockNode->GetUnlockedMessage();
    //Todo: figure out how to grab conditions. Prob have to be done while
    // linking?
}

void UGraphNodeDialogueOptionLock::RegenerateNodeConnections(UDialogueEdGraph* DialogueGraph)
{
    Super::RegenerateNodeConnections(DialogueGraph);

    //Copy over conditions
    //This is done here in case any of them need other graph nodes
    UDialogueOptionLockNode* OptionLockNode = 
        Cast<UDialogueOptionLockNode>(GetAssetNode());
    if (!OptionLockNode) return;

    Conditions.Empty();
    for (UDialogueCondition* Condition : OptionLockNode->GetConditions())
    {
        UDialogueGraphCondition* NewGraphCondition =
            NewObject<UDialogueGraphCondition>(this);
        UDialogueCondition* NewCondition =
            DuplicateObject<UDialogueCondition>(Condition, this);
        NewGraphCondition->SetCondition(
            NewCondition
        );

        Conditions.Add(NewGraphCondition);
    }
}

bool UGraphNodeDialogueOptionLock::GetIfAny() const
{
    return bIfAny;
}

TArray<FText> UGraphNodeDialogueOptionLock::GetConditionDisplayTexts() const
{
    TArray<FText> ConditionTexts;

    for (UDialogueGraphCondition* GraphCondition : Conditions)
    {
        if (GraphCondition == nullptr)
        {
            continue;
        }

        UDialogueQuery* Query = GraphCondition->GetQuery();
        UDialogueCondition* Condition = GraphCondition->GetCondition();

        if (Query && Condition && Condition->IsValidCondition())
        {
            ConditionTexts.Add(Condition->GetGraphDescription(
                Query->GetGraphDescription()
            ));
        }
        else
        {
            ConditionTexts.Add(LOCTEXT(
                "InvalidConditionText",
                "Invalid Condition"
            ));
        }
    }

    return ConditionTexts;
}

#undef LOCTEXT_NAMESPACE