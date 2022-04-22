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

#include "ItemType.hpp"
#include "RunningProcess.hpp"
#include "Settings.hpp"

#include <memory>
#include <string>
#include <vector>

namespace CaffeineTake {

struct Item
{
    std::wstring value;
    ItemType     type;
    int          icon;

    Item (std::wstring value, ItemType type = ItemType::Invalid, int icon = INVALID_ICON_ID)
        : value (value)
        , type  (type)
        , icon  (icon)
    {
    }
};

class ItemList
{
    std::vector<Item> mItems;

public:
    ItemList (std::shared_ptr<Settings> settings, std::shared_ptr<RunningProcessList> processList)
    {
        if (settings)
        {
            auto findIcon = [&](const std::wstring& name, ItemType type)
            {
                for (const auto& p : processList->Get())
                {
                    switch (type)
                    {
                    case ItemType::Name:
                        if (name == p.second.name)
                        {
                            return p.second.icon;
                        }
                        break;
                    case ItemType::Path:    
                        if (name == p.second.path)
                        {
                            return p.second.icon;
                        }
                        break;
                    case ItemType::Window:
                        if (name == p.second.window)
                        {
                            return p.second.icon;
                        }
                        break;
                    }
                }

                return INVALID_ICON_ID;
            };

            processList->Refresh();

            for (auto name : settings->Auto.ProcessNames)
            {
                auto icon = findIcon(name, ItemType::Name);
                mItems.push_back(Item(name, ItemType::Name, icon));
            }

            auto iconCachePtr = processList->GetIconCache();
            for (auto path : settings->Auto.ProcessPaths)
            {
                auto icon = iconCachePtr->Insert(path);
                mItems.push_back(Item(path, ItemType::Path, icon));
            }

            for (auto window : settings->Auto.WindowTitles)
            {
                auto icon = findIcon(window, ItemType::Window);
                mItems.push_back(Item(window, ItemType::Window, icon));
            }
        }
    }

          auto& GetItems ()       { return mItems; }
    const auto& GetItems () const { return mItems; }

    auto Push (Item item) -> bool
    {
        if (!Exists(item.value, item.type))
        {
            mItems.push_back(std::move(item));
            return true;
        }

        return false;
    }

    auto Remove (size_t index) -> bool
    {
        if (index < mItems.size())
        {
            mItems.erase(std::next(mItems.begin(), index));
            return true;
        }

        return false;
    }

    auto Find (const std::wstring& value, const ItemType& type) -> std::vector<Item>::iterator
    {
        auto index = 0;
        for (auto i : mItems)
        {
            if (i.type == type && i.value == value)
            {
                return std::next(mItems.begin(), index);
            }

            index += 1;
        }

        return mItems.end();
    }

    auto Exists (const std::wstring& value, const ItemType& type) -> bool
    {
        return Find(value, type) != mItems.end();
    }

    auto Count ()
    {
        return mItems.size();
    }
        
    auto ToSettings () -> Settings
    {
        auto settings = Settings();

        for (const auto& item : mItems)
        {
            switch (item.type)
            {
            case ItemType::Name:
                settings.Auto.ProcessNames.push_back(item.value);
                break;
            case ItemType::Path:
                settings.Auto.ProcessPaths.push_back(item.value);
                break;
            case ItemType::Window:
                settings.Auto.WindowTitles.push_back(item.value);
                break;
            }
        }

        return settings;
    }
};

} // namespace CaffeineTake
