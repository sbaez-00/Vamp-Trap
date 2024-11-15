// Copyright Zachary Brett, 2024. All rights reserved.


#include "Graph/DialogueGraphCondition.h"
#include "Conditionals/DialogueConditionBool.h"
#include "Conditionals/DialogueConditionFloat.h"
#include "Conditionals/DialogueConditionInt.h"
#include "Conditionals/Queries/Base/DialogueQueryBool.h"
#include "Conditionals/Queries/Base/DialogueQueryFloat.h"
#include "Conditionals/Queries/Base/DialogueQueryInt.h"
#include "Conditionals/Queries/NodeVisitedQuery.h"
#include "Dialogue.h"
#include "DialogueNodeSocket.h"
#include "Graph/DialogueEdGraph.h"
#include "Graph/Nodes/GraphNodeDialogue.h"
#include "LogDialogueTree.h"

void UDialogueGraphCondition::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);

    if (!Query)
    {
        return;
    }

    Query = DuplicateObject(Query, this);
    RefreshCondition();
}

void UDialogueGraphCondition::RefreshCondition()
{
    if (!Query)
    {
        Condition = nullptr;
        return;
    }

    if (Query->IsA(UDialogueQueryBool::StaticClass()))
    {
        Condition = NewObject<UDialogueConditionBool>(this);
        Condition->SetQuery(Query);
    }
    else if (Query->IsA(UDialogueQueryFloat::StaticClass()))
    {
        Condition = NewObject<UDialogueConditionFloat>(this);
        Condition->SetQuery(Query);
    }
    else if (Query->IsA(UDialogueQueryInt::StaticClass()))
    {
        Condition = NewObject<UDialogueConditionInt>(this);
        Condition->SetQuery(Query);
    }
    else
    {
        Condition = nullptr;
    }
}

void UDialogueGraphCondition::FinalizeCondition(UDialogue* InOuter)
{
    if (!Query) return;
    if (!Condition) return;
    if (!InOuter) return;

    //Change outer and set up
    Condition->SetDialogue(InOuter);

    //If a node visited query, update the node socket
    UNodeVisitedQuery* NodeQuery = Cast<UNodeVisitedQuery>(Query);
    if (NodeQuery)
    {
        HandleNodeVisitedQuery(NodeQuery);
    }
}

UDialogueQuery* UDialogueGraphCondition::GetQuery()
{
    return Query;
}

UDialogueCondition* UDialogueGraphCondition::GetCondition()
{
    return Condition;
}

bool UDialogueGraphCondition::ShouldRefreshCondition()
{
    if (!Condition)
    {
        if (Query)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    if (Query)
    {
        if (Query->IsA<UDialogueQueryBool>() 
            && !Condition->IsA<UDialogueConditionBool>())
        {
            return true;
        }
        
        if (Query->IsA<UDialogueQueryFloat>()
            && !Condition->IsA<UDialogueConditionFloat>())
        {
            return true;
        }
        
        if (Query->IsA<UDialogueQueryInt>()
            && !Condition->IsA<UDialogueConditionInt>())
        {
            return true;
        }
    }

    return false;
}

void UDialogueGraphCondition::SetCondition(UDialogueCondition* InCondition)
{
    Condition = InCondition;
    Query = InCondition->GetQuery();

    //If graph data is needed, update that 
    if (!Query) return;
    if (UNodeVisitedQuery* NodeVisitedQuery = Cast<UNodeVisitedQuery>(Query))
    {
        RegenerateNodeVisitedQuery(NodeVisitedQuery);
    }
}

void UDialogueGraphCondition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if (ShouldRefreshCondition())
    {
        RefreshCondition();
    }
}

void UDialogueGraphCondition::HandleNodeVisitedQuery(UNodeVisitedQuery* InQuery)
{
    if (!InQuery) return;
    UDialogueNodeSocket* TargetSocket = InQuery->GetSocket();

    if (!TargetSocket) return;
    if (!TargetSocket->GetGraphNode())
    {
        UE_LOG(
            LogDialogueTree,
            Warning,
            TEXT("Attempting to compile node visited query without setting target node")
        );
        return;
    };

    UGraphNodeDialogue* TargetGraphNode =
        Cast<UGraphNodeDialogue>(TargetSocket->GetGraphNode());

    check(TargetGraphNode);

    UDialogueNode* TargetDialogueNode = TargetGraphNode->GetAssetNode();

    check(TargetDialogueNode);

    TargetSocket->SetDialogueNode(TargetDialogueNode);
}

void UDialogueGraphCondition::RegenerateNodeVisitedQuery(
    UNodeVisitedQuery* InQuery
)
{
    if (!InQuery || !InQuery->GetSocket()) return;

    UDialogueEdGraph* Graph = GetTypedOuter<UDialogueEdGraph>();
    if (!Graph) return;
    
    UDialogueNode* TargetNode = InQuery->GetSocket()->GetDialogueNode();
    if (!TargetNode) return;

    InQuery->GetSocket()->SetGraphNode(
        Graph->GetNode(TargetNode->GetNodeID())
    );
}
