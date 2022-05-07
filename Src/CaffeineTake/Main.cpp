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

#include "AppInitInfo.hpp"
#include "CaffeineApp.hpp"
#include "InstanceGuard.hpp"
#include "Logger.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Check if application is not running already.
    auto guard = CaffeineTake::InstanceGuard();
    if (!guard.Protect())
    {
        MessageBoxW(
            0,
            L"Failed to create instance guard.",
            L"Initialization failed",
            MB_OK
        );
        return -1;
    }

    if (guard.IsOtherInstance())
    {
        return 1;
    }
    
    const auto info = CaffeineTake::GetAppInitInfo(hInstance);
    if (!info)
    {
        MessageBoxW(
            0,
            L"Failed to read executable path",
            L"Initialization failed",
            MB_OK
        );
        return -3;
    }

#if defined(FEATURE_CAFFEINETAKE_LOGGER)
    CaffeineTake::InitLogger(info.value().LogFilePath);
#endif

    auto caffeineTray = CaffeineTake::CaffeineApp(info.value());
    if (!caffeineTray.Init())
    {
        MessageBoxW(
            0,
            L"Failed to initialize CaffeineTake.\nCheck CaffeineTake.log for more information.",
            L"Initialization failed",
            MB_OK
        );
        return -2;
    }

    return static_cast<int>(caffeineTray.MainLoop());
}
