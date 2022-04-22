// CaffeineTake - Keep your computer awake.
// 
// Copyright (c) 2020-2021 VacuityBox
// Copyright (c) 2022      serverfailure71
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "IconCache.hpp"
#include "ItemType.hpp"

#include <string>
#include <utility>

namespace CaffeineTake {

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

} // namespace CaffeineTake
