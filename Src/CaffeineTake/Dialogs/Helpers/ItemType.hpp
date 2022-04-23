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
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

namespace CaffeineTake {

enum class ItemType : unsigned char
{
    Name    = 0,
    Path    = 1,
    Window  = 2,
    Invalid = 255
};

constexpr auto ItemTypeToString (ItemType type) -> std::wstring_view
{
    switch (type)
    {
    case ItemType::Name:    return L"Name";
    case ItemType::Path:    return L"Path";
    case ItemType::Window:  return L"Window";
    }

    return L"Invalid";
}

constexpr auto StringToItemType (std::wstring_view str) -> ItemType
{
    if (str == L"Name")   return ItemType::Name;
    if (str == L"Path")   return ItemType::Path;
    if (str == L"Window") return ItemType::Window;
        
    return ItemType::Invalid;
}

} // namespace CaffeineTake
