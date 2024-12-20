// Copyright Zachary Brett, 2024. All rights reserved.

//Header
#include "Graph/Nodes/GraphNodeDialogueBranch.h"
//Plugin
#include "Conditionals/DialogueCondition.h"
#include "Conditionals/Queries/Base/DialogueQuery.h"
#include "Dialogue.h"
#include "Graph/DialogueEdGraph.h"
#include "Graph/DialogueGraphCondition.h"
#include "Graph/Nodes/GraphNodeDialogue.h"
#include "Nodes/DialogueBranchNode.h"
//UE
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "GraphNodeDialogueBranch"

UGraphNodeDialogueBranch* UGraphNodeDialogueBranch::MakeTemplate(UObject* Outer)
{
    return NewObject<UGraphNodeDialogueBranch>(Outer);
}

FText UGraphNodeDialogueBranch::GetTooltipText() const
{
    return LOCTEXT(
        "NodeTooltip", 
        "If/Else; Branches based on a sequence of conditions"
    );
}

FName UGraphNodeDialogueBranch::GetBaseID() const
{
    return FName("Branch");
}

void UGraphNodeDialogueBranch::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, "MultipleNodes", FName());
    CreatePin(EGPD_Output, "MultipleNodes", FName());
    CreatePin(EGPD_Output, "MultipleNodes", FName());
}

void UGraphNodeDialogueBranch::PostPasteNode()
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

FText UGraphNodeDialogueBranch::GetPinMenuLabel(int32 PinIndex) const
{
    check(Pins.IsValidIndex(PinIndex));
    check(PinIndex < INVALID_PIN_INDEX && PinIndex >= 0);
    
    switch (PinIndex)
    {
    case 0: 
        return LOCTEXT("InputPinMenuLabel", "Input Pin");
    case 1: 
        return LOCTEXT("TruePinMenuLabel", "If Pin");
    default: 
        return LOCTEXT("FalsePinMenuLabel", "Else Pin");
    }
}

void UGraphNodeDialogueBranch::CreateAssetNode(UDialogue* InAsset)
{
    UDialogueBranchNode* NewNode =
        NewObject<UDialogueBranchNode>(InAsset);

    SetAssetNode(NewNode);
}

void UGraphNodeDialogueBranch::FinalizeAssetNode()
{
    //Get asset node
    UDialogueBranchNode* TargetBranch =
        CastChecked<UDialogueBranchNode>(GetAssetNode());
    
    //Get true/false nodes
    TArray<UEdGraphPin*> OutputPins = GetOutputPins();
    UDialogueNode* TrueNode = GetTrueNode(OutputPins);
    UDialogueNode* FalseNode = GetFalseNode(OutputPins);

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

    //Init data
    TargetBranch->InitBranchData(bIfAny, TrueNode, FalseNode, AssetConditions);
}

bool UGraphNodeDialogueBranch::CanCompileNode()
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

void UGraphNodeDialogueBranch::LoadNodeData(UDialogueNode* InNode)
{
    Super::LoadNodeData(InNode);

    UDialogueBranchNode* BranchNode = 
        CastChecked<UDialogueBranchNode>(InNode);

    bIfAny = BranchNode->GetIfAny();
}

void UGraphNodeDialogueBranch::RegenerateNodeConnections(
    UDialogueEdGraph* DialogueGraph
)
{
    if (!DialogueGraph)
    {
        return;
    }

    UDialogueBranchNode* BranchNode = 
        Cast<UDialogueBranchNode>(GetAssetNode());
    if (!BranchNode)
    {
        return;
    }

    TArray<UEdGraphPin*> OutputPins = GetOutputPins();
    checkf(
        OutputPins.Num() == 2,
        TEXT("Can't regen node links. Branch node does not have two output pins")
    );

    if (UDialogueNode* TrueNode = BranchNode->GetTrueNode())
    {
        UGraphNodeDialogue* TrueGraphNode =
            DialogueGraph->GetNode(TrueNode->GetNodeID());
        checkf(
            TrueGraphNode,
            TEXT("Node in dialogue graph missing editor equivalent.")
        );

        TArray<UEdGraphPin*> InputPins = TrueGraphNode->GetInputPins();
        checkf(
            InputPins.Num() == 1,
            TEXT("Failed to regenerate dialogue node links. Node has multiple input pins.")
        );

        //Link the true node to the "if" pin (first output pin)
        GetSchema()->TryCreateConnection(OutputPins[0], InputPins[0]);
    }

    if (UDialogueNode* FalseNode = BranchNode->GetFalseNode())
    {
        UGraphNodeDialogue* FalseGraphNode =
            DialogueGraph->GetNode(FalseNode->GetNodeID());
        checkf(
            FalseGraphNode,
            TEXT("Node in dialogue graph missing editor equivalent.")
        );

        TArray<UEdGraphPin*> InputPins = FalseGraphNode->GetInputPins();
        checkf(
            InputPins.Num() == 1,
            TEXT("Failed to regenerate dialogue node links. Node has multiple input pins.")
        );

        //Link the true node to the "else" pin (second output pin)
        GetSchema()->TryCreateConnection(OutputPins[1], InputPins[0]);
    }

    //Copy over conditions
    //This is done here in case any of them need other graph nodes
    Conditions.Empty();
    for (UDialogueCondition* Condition : BranchNode->GetConditions())
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

bool UGraphNodeDialogueBranch::GetIfAny() const
{
    return bIfAny;
}

TArray<FText> UGraphNodeDialogueBranch::GetConditionDisplayTexts() const
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

UDialogueNode* UGraphNodeDialogueBranch::GetTrueNode(
    TArray<UEdGraphPin*>& OutputPins) const
{
    check(OutputPins.Num() == 2);
    UEdGraphPin* TruePin = OutputPins[0];

    UGraphNodeDialogue* TrueGraphNode = GetFirstLinkedChild(TruePin);

    if (TrueGraphNode)
    {
        return TrueGraphNode->GetAssetNode();
    }

    return nullptr;
}

UDialogueNode* UGraphNodeDialogueBranch::GetFalseNode(
    TArray<UEdGraphPin*>& OutputPins) const
{
    check(OutputPins.Num() == 2);
    UEdGraphPin* FalsePin = OutputPins[1];

    UGraphNodeDialogue* FalseGraphNode = GetFirstLinkedChild(FalsePin);

    if (FalseGraphNode)
    {
        return FalseGraphNode->GetAssetNode();
    }

    return nullptr;
}

TArray<UDialogueCondition*> UGraphNodeDialogueBranch::GetConditions() const
{
    TArray<UDialogueCondition*> ConditionObjs;

    for (UDialogueGraphCondition* GraphCondition : Conditions)
    {
        UDialogueCondition* NextCondition = GraphCondition->GetCondition();

        if (NextCondition)
        {
            ConditionObjs.Add(NextCondition);
        }
    }

    return ConditionObjs;
}


UGraphNodeDialogue* UGraphNodeDialogueBranch::GetFirstLinkedChild(
    UEdGraphPin* InPin) const
{
    TArray<UGraphNodeDialogue*> PinChildren;
    GetPinChildren(InPin, PinChildren);

    if (PinChildren.Num() > 0)
    {
        /**
        * Note: For now, branch nodes are limited to a single output
        * per pin. If this ever changes, I will want to sort the 
        * linked nodes left to right and return the entire array. 
        * Obviously that would be a fairly large user design 
        * change. 
        */
        return PinChildren[0]; 
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE