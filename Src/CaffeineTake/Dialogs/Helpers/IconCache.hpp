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

#include <filesystem>
#include <map>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

constexpr static auto INVALID_ICON_ID = int{-1};

class IconCache final
{
    HIMAGELIST mImgList;
    int        mNextIndex;

    std::map<std::wstring, int> mIconMap;

public:
    IconCache ()
        : mNextIndex (0)
    {
        mImgList = ImageList_Create(
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            ILC_COLOR32,
            16,
            16
        );
    }

    ~IconCache ()
    {
        ImageList_Destroy(mImgList);
    }

    auto Insert (std::wstring name, HICON icon) -> int
    {
        if (icon == NULL)
        {
            return INVALID_ICON_ID;
        }

        const auto& cachedIcon = mIconMap.find(name);
        if (cachedIcon != mIconMap.end())
        {
            return cachedIcon->second;
        }

        ImageList_AddIcon(mImgList, icon);
        mIconMap.insert(std::make_pair(std::move(name), mNextIndex));
                
        return mNextIndex++;
    }

    auto Insert (fs::path path) -> int
    {
        auto pathStr = path.wstring();

        auto id = GetId(pathStr);
        if (id == INVALID_ICON_ID)
        {
            auto hInstance = GetModuleHandle(NULL);
            auto icon = ExtractIcon(hInstance, pathStr.c_str(), 0);
            if (icon != NULL && icon != reinterpret_cast<HICON>(1))
            {
                id = Insert(pathStr, icon);
                DestroyIcon(icon);
            }
        }

        return id;
    }

    auto GetId (const std::wstring name) -> int
    {
        const auto& cachedIcon = mIconMap.find(name);
        if (cachedIcon != mIconMap.end())
        {
            return cachedIcon->second;
        }

        return INVALID_ICON_ID;
    }

    auto GetImageList () -> HIMAGELIST&
    {
        return mImgList;
    }
};

} // namespace CaffeineTake
