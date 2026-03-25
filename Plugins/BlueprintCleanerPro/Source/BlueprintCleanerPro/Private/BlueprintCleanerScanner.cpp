#include "BlueprintCleanerScanner.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

bool FBlueprintCleanerScanner::BuildSnapshot(UBlueprint* Blueprint, FBCPBlueprintSnapshot& OutSnapshot)
{
    OutSnapshot = FBCPBlueprintSnapshot();

    if (!Blueprint)
    {
        return false;
    }

    OutSnapshot.BlueprintName = Blueprint->GetName();

    int32 ExecNodeCount = 0;

    // Ubergraph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph)
        {
            continue;
        }

        FBCPGraphInfo GraphInfo;
        ScanGraph(Graph, EBCPGraphType::EventGraph, GraphInfo, ExecNodeCount);
        OutSnapshot.TotalNodeCount += GraphInfo.Nodes.Num();
        OutSnapshot.Graphs.Add(MoveTemp(GraphInfo));
    }

    // Function
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        FBCPGraphInfo GraphInfo;
        ScanGraph(Graph, EBCPGraphType::Function, GraphInfo, ExecNodeCount);
        OutSnapshot.TotalNodeCount += GraphInfo.Nodes.Num();
        OutSnapshot.Graphs.Add(MoveTemp(GraphInfo));
    }

    // Macro
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        FBCPGraphInfo GraphInfo;
        ScanGraph(Graph, EBCPGraphType::Macro, GraphInfo, ExecNodeCount);
        OutSnapshot.TotalNodeCount += GraphInfo.Nodes.Num();
        OutSnapshot.Graphs.Add(MoveTemp(GraphInfo));
    }

    OutSnapshot.TotalExecNodeCount = ExecNodeCount;
    return true;
}

void FBlueprintCleanerScanner::ScanGraph(
    UEdGraph* Graph,
    EBCPGraphType GraphType,
    FBCPGraphInfo& OutGraphInfo,
    int32& OutExecNodeCount)
{
    if (!Graph)
    {
        return;
    }

    OutGraphInfo.GraphName = Graph->GetName();
    OutGraphInfo.GraphType = GraphType;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        FBCPNodeInfo NodeInfo;
        NodeInfo.Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
        NodeInfo.ClassName = Node->GetClass()->GetName();
        NodeInfo.Position = FVector2D(Node->NodePosX, Node->NodePosY);
        //添加节点id
        NodeInfo.NodeId = Node->NodeGuid.IsValid()
            ? Node->NodeGuid.ToString()
            : FString::Printf(TEXT("%s_%p"), *Node->GetName(), Node);

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin)
            {
                continue;
            }

            if (Pin->Direction == EGPD_Input)
            {
                ++NodeInfo.InputPinCount;
            }
            else if (Pin->Direction == EGPD_Output)
            {
                ++NodeInfo.OutputPinCount;
            }
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == TEXT("exec"))
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (!LinkedPin || !LinkedPin->GetOwningNode())
                    {
                        continue;
                    }

                    //连接的节点id
                    UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
                    const FString LinkedNodeId = LinkedNode->NodeGuid.IsValid()
                        ? LinkedNode->NodeGuid.ToString()
                        : FString::Printf(TEXT("%s_%p"), *LinkedNode->GetName(), LinkedNode);
                    NodeInfo.ExecOutputNodeIds.AddUnique(LinkedNodeId);
                }
            }
        }

        NodeInfo.bHasExecPin = NodeHasExecPin(Node);
        if (NodeInfo.bHasExecPin)
        {
            ++OutExecNodeCount;
        }

        const bool bHasExecOutputPin = NodeHasExecOutputPin(Node);
        const bool bHasConnectedExecInput = NodeHasConnectedExecInput(Node);

        NodeInfo.bIsExecEntryCandidate = bHasExecOutputPin && !bHasConnectedExecInput;

        OutGraphInfo.Nodes.Add(MoveTemp(NodeInfo));
    }
}

bool FBlueprintCleanerScanner::NodeHasExecInputPin(const UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == TEXT("exec"))
        {
            return true;
        }
    }

    return false;
}

bool FBlueprintCleanerScanner::NodeHasConnectedExecInput(const UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == TEXT("exec"))
        {
            if (Pin->LinkedTo.Num() > 0)
            {
                return true;
            }
        }
    }

    return false;
}

bool FBlueprintCleanerScanner::NodeHasExecOutputPin(const UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == TEXT("exec"))
        {
            return true;
        }
    }

    return false;
}

bool FBlueprintCleanerScanner::NodeHasExecPin(const UEdGraphNode* Node)
{
    return NodeHasExecInputPin(Node) || NodeHasExecOutputPin(Node);
}