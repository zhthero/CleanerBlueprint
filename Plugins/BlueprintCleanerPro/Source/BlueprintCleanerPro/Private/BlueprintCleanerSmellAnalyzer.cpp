#include "BlueprintCleanerSmellAnalyzer.h"

void FBlueprintCleanerSmellAnalyzer::AnalyzeLongExecutionChains(
    const FBCPBlueprintSnapshot& Snapshot,
    TArray<FBCPSmellInfo>& OutSmells,
    int32 ChainLengthThreshold)
{
    for (const FBCPGraphInfo& GraphInfo : Snapshot.Graphs)
    {
        if (GraphInfo.GraphType != EBCPGraphType::EventGraph)
        {
            continue;
        }

        AnalyzeGraphForLongExecutionChain(GraphInfo, OutSmells, ChainLengthThreshold);
    }
}

void FBlueprintCleanerSmellAnalyzer::AnalyzeGraphForLongExecutionChain(
    const FBCPGraphInfo& GraphInfo,
    TArray<FBCPSmellInfo>& OutSmells,
    int32 ChainLengthThreshold)
{
    TMap<FString, const FBCPNodeInfo*> NodeMap;
    for (const FBCPNodeInfo& Node : GraphInfo.Nodes)
    {
        NodeMap.Add(Node.NodeId, &Node);
    }

    int32 BestLength = 0;
    const FBCPNodeInfo* BestStartNode = nullptr;

    TMap<FString, int32> Memo;

    for (const FBCPNodeInfo& Node : GraphInfo.Nodes)
    {
        if (!Node.bIsExecEntryCandidate)
        {
            continue;
        }

        TSet<FString> Visiting;
        const int32 Length = GetLongestExecPathFromNode(Node.NodeId, NodeMap, Visiting, Memo);

        if (Length > BestLength)
        {
            BestLength = Length;
            BestStartNode = &Node;
        }
    }

    if (BestLength >= ChainLengthThreshold)
    {
        FBCPSmellInfo Smell;
        Smell.SmellType = EBCPSmellType::LongExecutionChain;
        Smell.GraphName = GraphInfo.GraphName;
        Smell.Score = BestLength;
        Smell.Message = FString::Printf(
            TEXT("检测到过长执行链，最长链长度为 %d，建议拆分为函数、宏或注释分组。"),
            BestLength);

        if (BestStartNode)
        {
            Smell.RelatedNodeTitles.Add(BestStartNode->Title);
        }

        OutSmells.Add(MoveTemp(Smell));
    }
}

int32 FBlueprintCleanerSmellAnalyzer::GetLongestExecPathFromNode(
    const FString& NodeId,
    const TMap<FString, const FBCPNodeInfo*>& NodeMap,
    TSet<FString>& Visiting,
    TMap<FString, int32>& Memo)
{
    if (const int32* Cached = Memo.Find(NodeId))
    {
        return *Cached;
    }

    if (Visiting.Contains(NodeId))
    {
        // 避免环导致无限递归
        return 0;
    }

    const FBCPNodeInfo* Node = FindNodeById(NodeId, NodeMap);
    if (!Node)
    {
        return 0;
    }

    Visiting.Add(NodeId);

    int32 BestChildLength = 0;

    for (const FString& NextNodeId : Node->ExecOutputNodeIds)
    {
        const int32 ChildLength = GetLongestExecPathFromNode(NextNodeId, NodeMap, Visiting, Memo);
        BestChildLength = FMath::Max(BestChildLength, ChildLength);
    }

    Visiting.Remove(NodeId);

    const int32 Result = 1 + BestChildLength;
    Memo.Add(NodeId, Result);
    return Result;
}

const FBCPNodeInfo* FBlueprintCleanerSmellAnalyzer::FindNodeById(
    const FString& NodeId,
    const TMap<FString, const FBCPNodeInfo*>& NodeMap)
{
    if (const FBCPNodeInfo* const* Found = NodeMap.Find(NodeId))
    {
        return *Found;
    }

    return nullptr;
}