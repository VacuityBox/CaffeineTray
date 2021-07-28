#pragma once

#include "Dialog.hpp"
#include "Helpers/DialogResult.hpp"
#include "Helpers/IconCache.hpp"
#include "Helpers/ItemList.hpp"
#include "Helpers/RunningProcess.hpp"
#include "Settings.hpp"
#include "resource.h"

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <windowsx.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

class AddWizardDialog final : public Dialog<AddWizardResult>
{
    std::shared_ptr<ItemList>           mItems;
    std::shared_ptr<IconCache>          mIconCache;
    std::shared_ptr<RunningProcessList> mRunningProcesses;

    virtual auto OnInit    (HWND dlgHandle)               -> bool override;
    virtual auto OnOk      ()                             -> bool override;
    virtual auto OnCancel  ()                             -> bool override;
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnClose   ()                             -> void override;

    auto Refresh (HWND listView,  bool select = false) -> void;

public:
    AddWizardDialog (
        std::shared_ptr<ItemList>           items,
        std::shared_ptr<IconCache>          iconCache,
        std::shared_ptr<RunningProcessList> processList
    )
        : Dialog            (IDD_ADD_WIZARD)
        , mItems            (items)
        , mIconCache        (iconCache)
        , mRunningProcesses (processList)
    {
    }
};

} // namespace Caffeine
