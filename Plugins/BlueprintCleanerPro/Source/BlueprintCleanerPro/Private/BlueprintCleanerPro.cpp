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
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
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

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        BlueprintCleanerProTabName,
        FOnSpawnTab::CreateRaw(this, &FBlueprintCleanerProModule::OnSpawnPluginTab))
        .SetDisplayName(FText::FromString("Blueprint Cleaner Pro"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintCleanerProModule::RegisterMenus));

    CachedReportText = TEXT("Blueprint Cleaner Pro Ready\n\n点击工具栏按钮开始扫描当前活跃蓝图。");

}

void FBlueprintCleanerProModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FBlueprintCleanerProStyle::Shutdown();

	FBlueprintCleanerProCommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BlueprintCleanerProTabName);
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
        CachedReportText = TEXT("扫描失败：无法构建 Blueprint Snapshot。");
        RefreshReportWidget();
        FGlobalTabmanager::Get()->TryInvokeTab(BlueprintCleanerProTabName);
        return;
    }

    TArray<FBCPSmellInfo> Smells;
    FBlueprintCleanerSmellAnalyzer::AnalyzeLongExecutionChains(Snapshot, Smells, 8);

    FString Report;
    Report += FString::Printf(TEXT("Blueprint: %s\n"), *Snapshot.BlueprintName);
    Report += FString::Printf(TEXT("Graphs: %d\n"), Snapshot.Graphs.Num());
    Report += FString::Printf(TEXT("Total Nodes: %d\n"), Snapshot.TotalNodeCount);
    Report += FString::Printf(TEXT("Exec Nodes: %d\n"), Snapshot.TotalExecNodeCount);
    Report += FString::Printf(TEXT("Smells: %d\n\n"), Smells.Num());

    Report += TEXT("=== Graphs ===\n");
    for (const FBCPGraphInfo& GraphInfo : Snapshot.Graphs)
    {
        const TCHAR* GraphTypeText = TEXT("Unknown");

        switch (GraphInfo.GraphType)
        {
        case EBCPGraphType::EventGraph:
            GraphTypeText = TEXT("EventGraph");
            break;
        case EBCPGraphType::Function:
            GraphTypeText = TEXT("Function");
            break;
        case EBCPGraphType::Macro:
            GraphTypeText = TEXT("Macro");
            break;
        default:
            break;
        }

        Report += FString::Printf(
            TEXT("- %s | Type: %s | Nodes: %d\n"),
            *GraphInfo.GraphName,
            GraphTypeText,
            GraphInfo.Nodes.Num());
    }

    Report += TEXT("\n=== Smells ===\n");
    if (Smells.Num() == 0)
    {
        Report += TEXT("No smells detected.\n");
    }
    else
    {
        for (const FBCPSmellInfo& Smell : Smells)
        {
            const TCHAR* SmellTypeText = TEXT("Unknown");

            switch (Smell.SmellType)
            {
            case EBCPSmellType::LongExecutionChain:
                SmellTypeText = TEXT("LongExecutionChain");
                break;
            default:
                break;
            }

            Report += FString::Printf(
                TEXT("[%s]\nGraph: %s\nScore: %d\nMessage: %s\n"),
                SmellTypeText,
                *Smell.GraphName,
                Smell.Score,
                *Smell.Message);

            if (Smell.RelatedNodeTitles.Num() > 0)
            {
                Report += TEXT("Related Nodes:\n");
                for (const FString& NodeTitle : Smell.RelatedNodeTitles)
                {
                    Report += FString::Printf(TEXT("  - %s\n"), *NodeTitle);
                }
            }

            Report += TEXT("\n");
        }
    }

    CachedReportText = MoveTemp(Report);
    FGlobalTabmanager::Get()->TryInvokeTab(BlueprintCleanerProTabName);
    RefreshReportWidget();
}

TSharedRef<SDockTab> FBlueprintCleanerProModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder)
				.Padding(8.0f)
				[
					SAssignNew(ReportTextBox, SMultiLineEditableTextBox)
						.IsReadOnly(true)
						.Text(FText::FromString(CachedReportText.IsEmpty()
							? TEXT("Blueprint Cleaner Pro Ready\n\n点击工具栏按钮开始扫描当前活跃蓝图。")
							: CachedReportText))
				]
		];
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

FString FBlueprintCleanerProModule::BuildReportText() const
{
	return CachedReportText;
}

void FBlueprintCleanerProModule::RefreshReportWidget()
{
	if (ReportTextBox.IsValid())
	{
		ReportTextBox->SetText(FText::FromString(CachedReportText));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintCleanerProModule, BlueprintCleanerPro)