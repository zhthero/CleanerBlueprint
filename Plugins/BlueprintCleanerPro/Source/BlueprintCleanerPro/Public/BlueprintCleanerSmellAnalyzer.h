#pragma once

#include "CoreMinimal.h"
#include "BlueprintCleanerTypes.h"

class FBlueprintCleanerSmellAnalyzer
{
public:
    static void AnalyzeLongExecutionChains(
        const FBCPBlueprintSnapshot& Snapshot,
        TArray<FBCPSmellInfo>& OutSmells,
        int32 ChainLengthThreshold = 8);

private:
    static void AnalyzeGraphForLongExecutionChain(
        const FBCPGraphInfo& GraphInfo,
        TArray<FBCPSmellInfo>& OutSmells,
        int32 ChainLengthThreshold);

    static int32 GetLongestExecPathFromNode(
        const FString& NodeId,
        const TMap<FString, const FBCPNodeInfo*>& NodeMap,
        TSet<FString>& Visiting,
        TMap<FString, int32>& Memo);

    static const FBCPNodeInfo* FindNodeById(
        const FString& NodeId,
        const TMap<FString, const FBCPNodeInfo*>& NodeMap);
};