// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "BlueprintCleanerProStyle.h"

class FBlueprintCleanerProCommands : public TCommands<FBlueprintCleanerProCommands>
{
public:

	FBlueprintCleanerProCommands()
		: TCommands<FBlueprintCleanerProCommands>(TEXT("BlueprintCleanerPro"), NSLOCTEXT("Contexts", "BlueprintCleanerPro", "BlueprintCleanerPro Plugin"), NAME_None, FBlueprintCleanerProStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
