#include "BlueprintCleanerEditorUtils.h"

#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Misc/MessageDialog.h"

namespace BlueprintCleanerEditorUtils
{
    UBlueprint* GetMostRecentlyActivatedBlueprintEditorAsset()
    {
        if (!GEditor)
        {
            return nullptr;
        }

        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (!AssetEditorSubsystem)
        {
            return nullptr;
        }

        const TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();

        UBlueprint* BestBlueprint = nullptr;
        double BestActivationTime = -1.0;

        for (UObject* Asset : EditedAssets)
        {
            if (!Asset)
            {
                continue;
            }

            UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
            if (!Blueprint)
            {
                continue;
            }

            IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
            if (!EditorInstance)
            {
                continue;
            }

            const double ActivationTime = EditorInstance->GetLastActivationTime();
            if (ActivationTime > BestActivationTime)
            {
                BestActivationTime = ActivationTime;
                BestBlueprint = Blueprint;
            }
        }

        return BestBlueprint;
    }

    void ShowNoActiveBlueprintDialog()
    {
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(TEXT("当前无活跃蓝图"))
        );
    }
}