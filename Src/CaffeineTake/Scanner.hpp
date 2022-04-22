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

#include "Settings.hpp"
#include "Utility.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Scanner
{
public:
    virtual ~Scanner() {}

    virtual auto Run () -> bool = 0;
};

class ProcessScanner : public Scanner
{
    std::shared_ptr<Settings> mSettingsPtr;
    std::wstring              mLastProcessName;
    std::wstring              mLastProcessPath;
    DWORD                     mLastPid;

    auto CheckLast () -> bool
    {
        auto path = GetProcessPath(mLastPid);
        if (!path.empty())
        {
            if (mLastProcessPath.empty())
            {
                return path.filename() == mLastProcessName;
            }
            else
            {
                return path == mLastProcessPath;
            }
        }

        return false;
    }

public:
    ProcessScanner (std::shared_ptr<Settings> settings)
        : mSettingsPtr (settings)
        , mLastPid     (0)
    {
    }

    auto Run () -> bool override
    {
        if (mSettingsPtr->Auto.ProcessNames.empty() && mSettingsPtr->Auto.ProcessPaths.empty())
        {
            return false;
        }

        // Only check last.
        if (mLastPid != 0)
        {
            if (CheckLast())
            {
                return true;
            }
        }

        mLastProcessName.clear();
        mLastProcessPath.clear();
        mLastPid = 0;
        return ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                // Check if process is on process names list.
                for (auto procName : mSettingsPtr->Auto.ProcessNames)
                {
                    if (procName == path.filename())
                    {
                        mLastProcessName = procName;
                        mLastPid         = pid;
                        return true;
                    }
                }

                // Check if process is on process paths list.
                for (auto procPath : mSettingsPtr->Auto.ProcessPaths)
                {
                    if (procPath == path)
                    {
                        mLastProcessPath = procPath;
                        mLastPid         = pid;
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }

    const auto& GetLastFound()
    {
        return mLastProcessPath.empty() ? mLastProcessName : mLastProcessPath;
    }
};

class WindowScanner : public Scanner
{
    std::shared_ptr<Settings> mSettingsPtr;
    std::wstring              mLastFound;

public:
    WindowScanner (std::shared_ptr<Settings> settings)
        : mSettingsPtr (settings)
    {
    }

    auto Run () -> bool override
    {
        mLastFound = L"";
        return ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view window)
            {
                // Check if process is on window title list.
                for (auto windowTitle : mSettingsPtr->Auto.WindowTitles)
                {
                    if (windowTitle == window)
                    {
                        mLastFound = window;
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }

    const auto& GetLastFound() { return mLastFound; }
};

} // namespace CaffeineTake
