#pragma once

#include "CoreMinimal.h"
#include "BlueprintCleanerTypes.h"

class UBlueprint;
class UEdGraph;
class UEdGraphNode;

class FBlueprintCleanerScanner
{
public:
    static bool BuildSnapshot(UBlueprint* Blueprint, FBCPBlueprintSnapshot& OutSnapshot);

private:
    static bool NodeHasExecPin(const UEdGraphNode* Node);
    static void ScanGraph(
        UEdGraph* Graph,
        EBCPGraphType GraphType,
        FBCPGraphInfo& OutGraphInfo,
        int32& OutExecNodeCount);
    static bool NodeHasExecInputPin(const UEdGraphNode* Node);
    static bool NodeHasConnectedExecInput(const UEdGraphNode* Node);
    static bool NodeHasExecOutputPin(const UEdGraphNode* Node);
};