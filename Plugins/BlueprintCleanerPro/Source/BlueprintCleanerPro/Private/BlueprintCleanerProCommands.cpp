// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintCleanerProCommands.h"

#define LOCTEXT_NAMESPACE "FBlueprintCleanerProModule"

void FBlueprintCleanerProCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "BlueprintCleanerPro", "Execute BlueprintCleanerPro action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
