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

#include "AppInitInfo.hpp"

#include "Utility.hpp"
#include "Version.hpp"

#include <array>
#include <vector>

namespace CaffeineTake {

auto GetAppInitInfo (HINSTANCE hInstance) -> AppInitInfo
{
    auto stackBuffer = std::array<wchar_t, MAX_PATH>();
    stackBuffer.fill(L'\0');

    auto ptr  = stackBuffer.data();
    auto size = stackBuffer.size();

    auto heapBuffer = std::vector<wchar_t>();

    // Get executable path.
    auto readExecutablePath = false;
    while (true)
    {
        auto ret = GetModuleFileNameW(NULL, ptr, size);
        if (ret == 0)
        {
            break;
        }

        if (ret == size)
        {
            size = size * 2;
            heapBuffer.resize(size);

            ptr = heapBuffer.data();

            continue;
        }

        readExecutablePath = true;
        break;
    }

    auto root = fs::path();
    auto settings = fs::path();

    auto isPortable = false;

    // If reading executable path failed, we use %appdata% as fallback.
    if (readExecutablePath)
    {
        const auto exe = fs::path(ptr);
        
        root = exe.root_directory();
        settings = root / CAFFEINE_TAKE_PORTABLE_SETTINGS_FILENAME;

        if (fs::exists(settings))
        {
            isPortable = true;
        }
    }

    if (!isPortable)
    {
        auto appData = GetAppDataPath() / CAFFEINE_TAKE_PROGRAM_NAME;
        fs::create_directory(appData);

        root = std::move(appData);
        settings = root / CAFFEINE_TAKE_SETTINGS_FILENAME;
    }

    return AppInitInfo{
        .InstanceHandle = hInstance ? hInstance : GetModuleHandleW(NULL),
        .DataDirectory  = root,
        .SettingsPath   = settings,
        .LogFilePath    = root / CAFFEINE_TAKE_LOG_FILENAME,
        .IsPortable     = isPortable
    };
}

} // namespace CaffeineTake
