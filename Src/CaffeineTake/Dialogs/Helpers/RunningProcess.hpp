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

#include "IconCache.hpp"
#include "Utility.hpp"

#include <string>
#include <utility>
#include <vector>

namespace CaffeineTake {

struct ProcessInfo
{
    std::wstring path;
    std::wstring name;
    std::wstring window;
    int          icon;

    ProcessInfo (std::wstring path, std::wstring name, std::wstring window, int icon)
        : path   (path)
        , name   (name)
        , window (window)
        , icon   (icon)
    {
    }
};

class RunningProcessList
{
    std::vector<std::pair<int, ProcessInfo>> mRunningProcesses;
    std::shared_ptr<IconCache>               mIconCache;

public:
    RunningProcessList (std::shared_ptr<IconCache> iconCache)
        : mIconCache (iconCache)
    {
    }

          auto& Get ()       { return mRunningProcesses; }
    const auto& Get () const { return mRunningProcesses; }

    auto GetIconCache () { return mIconCache; }

    auto Refresh () -> void
    {
        mRunningProcesses.clear();

        // Load list of running processes.
        ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                auto icon = mIconCache->Insert(path);
                mRunningProcesses.push_back(
                    std::make_pair(
                        pid,
                        ProcessInfo(path.wstring(), path.filename().wstring(), std::wstring(), icon)
                    )
                );

                return false; // don't stop iterating
            }
        );

        // Load window titles.
        ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view title)
            {
                for (auto& process : mRunningProcesses)
                {
                    if (process.first == pid)
                    {
                        process.second.window = title;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

} // namespace CaffeineTake
