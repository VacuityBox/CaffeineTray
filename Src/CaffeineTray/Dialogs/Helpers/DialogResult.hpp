#pragma once

#include "IconCache.hpp"
#include "ItemType.hpp"

#include <string>
#include <utility>

namespace Caffeine {

struct AddDialogResult
{
    std::wstring value;
    ItemType     type;

    AddDialogResult ()
        : type (ItemType::Invalid)
    {
    }
};

using EditDialogResult = AddDialogResult;

struct AddWizardResult
{
    std::wstring value;
    ItemType     type;
    int          icon;

    AddWizardResult ()
        : type (ItemType::Invalid)
        , icon (INVALID_ICON_ID)
    {
    }
};

} // namespace Caffeine
