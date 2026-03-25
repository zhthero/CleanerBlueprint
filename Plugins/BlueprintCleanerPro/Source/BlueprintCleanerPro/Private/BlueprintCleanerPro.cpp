// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintCleanerPro.h"
#include "BlueprintCleanerProStyle.h"
#include "BlueprintCleanerProCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Editor/UnrealEd/Public/Selection.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/UObjectIterator.h"
#include "BlueprintEditor.h"
#include "Engine/Blueprint.h"

#include "BlueprintCleanerScanner.h"
#include "BlueprintCleanerEditorUtils.h"
#include "BlueprintCleanerSmellAnalyzer.h"

static const FName BlueprintCleanerProTabName("BlueprintCleanerPro");

#define LOCTEXT_NAMESPACE "FBlueprintCleanerProModule"

void FBlueprintCleanerProModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FBlueprintCleanerProStyle::Initialize();
	FBlueprintCleanerProStyle::ReloadTextures();

	FBlueprintCleanerProCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FBlueprintCleanerProCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FBlueprintCleanerProModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintCleanerProModule::RegisterMenus));
}

void FBlueprintCleanerProModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FBlueprintCleanerProStyle::Shutdown();

	FBlueprintCleanerProCommands::Unregister();
}

static const TCHAR* GraphTypeToString(EBCPGraphType GraphType)
{
	switch (GraphType)
	{
	case EBCPGraphType::EventGraph:
		return TEXT("EventGraph");
	case EBCPGraphType::Function:
		return TEXT("Function");
	case EBCPGraphType::Macro:
		return TEXT("Macro");
	default:
		return TEXT("Unknown");
	}
}

void FBlueprintCleanerProModule::PluginButtonClicked()
{
	UBlueprint* Blueprint = BlueprintCleanerEditorUtils::GetMostRecentlyActivatedBlueprintEditorAsset();

	if (!Blueprint)
	{
		BlueprintCleanerEditorUtils::ShowNoActiveBlueprintDialog();
		return;
	}

	FBCPBlueprintSnapshot Snapshot;
	if (!FBlueprintCleanerScanner::BuildSnapshot(Blueprint, Snapshot))
	{
		UE_LOG(LogTemp, Warning, TEXT("BCP: Failed to build snapshot"));
		return;
	}

	TArray<FBCPSmellInfo> Smells;
	FBlueprintCleanerSmellAnalyzer::AnalyzeLongExecutionChains(Snapshot, Smells, 8);

	UE_LOG(LogTemp, Warning, TEXT("=== Long Execution Chain Analysis ==="));
	UE_LOG(LogTemp, Warning, TEXT("Blueprint: %s"), *Snapshot.BlueprintName);
	UE_LOG(LogTemp, Warning, TEXT("Smell Count: %d"), Smells.Num());

	for (const FBCPSmellInfo& Smell : Smells)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Smell] Graph=%s | Score=%d | %s"),
			*Smell.GraphName,
			Smell.Score,
			*Smell.Message);

		for (const FString& NodeTitle : Smell.RelatedNodeTitles)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Related Node: %s"), *NodeTitle);
		}
	}
	for (const FBCPGraphInfo& GraphInfo : Snapshot.Graphs)
	{
		UE_LOG(LogTemp, Warning, TEXT("Graph: %s"), *GraphInfo.GraphName);

		for (const FBCPNodeInfo& NodeInfo : GraphInfo.Nodes)
		{
			if (NodeInfo.bIsExecEntryCandidate)
			{
				UE_LOG(LogTemp, Warning, TEXT("  Entry Candidate: %s"), *NodeInfo.Title);
			}
		}
	}
}

void FBlueprintCleanerProModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FBlueprintCleanerProCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FBlueprintCleanerProCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintCleanerProModule, BlueprintCleanerPro)