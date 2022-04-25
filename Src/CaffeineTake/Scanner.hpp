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

#include "Settings.hpp"
#include "Utility.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
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

    virtual auto Run (SettingsPtr) -> bool = 0;
};

class ProcessScanner : public Scanner
{
    std::wstring mLastProcessName = L"";
    std::wstring mLastProcessPath = L"";
    DWORD        mLastPid         = 0;

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
    virtual auto Run (SettingsPtr settings) -> bool override
    {
        if (settings->Auto.ProcessNames.empty() && settings->Auto.ProcessPaths.empty())
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

            const auto& last = mLastProcessPath.empty() ? mLastProcessName : mLastProcessPath;
            spdlog::info(L"Process: {} (PID: {}), no longer exists, scanning all processes", last, mLastPid);
        }

        mLastProcessName.clear();
        mLastProcessPath.clear();
        mLastPid = 0;

        return ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                // Check if process is on process names list.
                for (auto procName : settings->Auto.ProcessNames)
                {
                    if (procName == path.filename())
                    {
                        mLastProcessName = procName;
                        mLastPid         = pid;

                        spdlog::info(L"Found process: {} (PID: {})", procName, pid);
                        return true;
                    }
                }

                // Check if process is on process paths list.
                for (auto procPath : settings->Auto.ProcessPaths)
                {
                    if (procPath == path)
                    {
                        mLastProcessPath = procPath;
                        mLastPid         = pid;

                        spdlog::info(L"Found process: {} (PID: {})", procPath, pid);
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

class WindowScanner : public Scanner
{
public:
    auto Run (SettingsPtr settings) -> bool override
    {
        return ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view window)
            {
                // Check if process is on window title list.
                for (auto windowTitle : settings->Auto.WindowTitles)
                {
                    if (windowTitle == window)
                    {
                        spdlog::info(L"Found window: {} (PID: {})", windowTitle, pid);
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

} // namespace CaffeineTake
