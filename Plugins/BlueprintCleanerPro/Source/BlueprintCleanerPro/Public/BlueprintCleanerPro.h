// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class SMultiLineEditableTextBox;

class FBlueprintCleanerProModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
    void PluginButtonClicked();
    TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

    void RegisterMenus();
    FString BuildReportText() const;
    void RefreshReportWidget();

private:
    TSharedPtr<class FUICommandList> PluginCommands;
    TSharedPtr<SMultiLineEditableTextBox> ReportTextBox;
    FString CachedReportText;
};
