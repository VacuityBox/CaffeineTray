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

#include "PCH.hpp"

#include "AppInitInfo.hpp"
#include "CaffeineApp.hpp"
#include "CommandLineArgs.hpp"
#include "InstanceGuard.hpp"
#include "Logger.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

auto WINAPI wWinMain (
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nShowCmd
) -> int
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    // Protect the instance with guard.
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

    // Parse command line.
    auto args = CaffeineTake::ParseCommandLine(lpCmdLine);
    
    // Check if application is not running already.
    if (guard.IsOtherInstance())
    {
        // Main instance is running, so if there is Task to be executed send it.
        if (args.Task != CaffeineTake::Task::Invalid())
        {
            CaffeineTake::CaffeineApp::SendMessageToMainInstance(
                CaffeineTake::WM_CAFFEINE_TAKE_SECOND_INSTANCE_MESSAGE,
                static_cast<WPARAM>(args.Task.MessageId),
                NULL
            );
        }

        return 1;
    }
    
    const auto info = CaffeineTake::GetAppInitInfo(hInstance, args);
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
    if (!caffeineTray.Init(info.value()))
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
