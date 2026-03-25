#pragma once

#include "CoreMinimal.h"

enum class EBCPGraphType : uint8
{
    EventGraph,
    Function,
    Macro,
    Unknown
};

enum class EBCPSmellType : uint8
{
    LongExecutionChain,
    DuplicateLogic,
    GodBlueprint,
    LowCommentCoverage,
    Unknown
};

struct FBCPNodeInfo
{
    FString NodeId;
    FString Title;
    FString ClassName;
    FVector2D Position = FVector2D::ZeroVector;
    int32 InputPinCount = 0;
    int32 OutputPinCount = 0;
    bool bHasExecPin = false;

    bool bIsExecEntryCandidate = false;

    TArray<FString> ExecOutputNodeIds;
};

struct FBCPGraphInfo
{
    FString GraphName;
    EBCPGraphType GraphType = EBCPGraphType::Unknown;
    TArray<FBCPNodeInfo> Nodes;
};

struct FBCPBlueprintSnapshot
{
    FString BlueprintName;
    TArray<FBCPGraphInfo> Graphs;
    int32 TotalNodeCount = 0;
    int32 TotalExecNodeCount = 0;
};

struct FBCPSmellInfo
{
    EBCPSmellType SmellType = EBCPSmellType::Unknown;
    FString GraphName;
    FString Message;
    int32 Score = 0;
    TArray<FString> RelatedNodeTitles;
};