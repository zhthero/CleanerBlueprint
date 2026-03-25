#pragma once

#include "CoreMinimal.h"

class UBlueprint;

namespace BlueprintCleanerEditorUtils
{
    UBlueprint* GetMostRecentlyActivatedBlueprintEditorAsset();
    void ShowNoActiveBlueprintDialog();
}