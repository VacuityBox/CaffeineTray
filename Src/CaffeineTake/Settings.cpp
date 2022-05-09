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

#include "Config.hpp"
#include "Settings.hpp"

#if defined(FEATURE_CAFFEINETAKE_SETTINGS)
#   include "Logger.hpp"
#   include "StringSerializer.hpp"
#   include <filesystem>
#   include <fstream>
#   include <string>
#   include <nlohmann/json.hpp>
#endif
namespace CaffeineTake {

#if defined(FEATURE_CAFFEINETAKE_SETTINGS)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(struct Settings::General, IconPack, LangId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(struct Settings::Standard, KeepDisplayOn, DisableOnLockScreen)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    struct Settings::Auto,
    KeepDisplayOn,
    DisableOnLockScreen,
    ScanInterval,
    ProcessNames,
    ProcessPaths,
    WindowTitles,
    ScheduleEntries,
    UsbDevices,
    BluetoothDevices,
    ActiveTimeout
)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(struct Settings::Timer, KeepDisplayOn, DisableOnLockScreen, Interval)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings, General, Standard, Auto, Timer)

#endif

auto Settings::Load (const fs::path& path) -> bool
{
#if defined(FEATURE_CAFFEINETAKE_SETTINGS)
    // NOTE: Settings should be in UTF-8
    // Open settings file for read.
    auto file = std::ifstream(path);
    if (!file)
    {
        LOG_ERROR(L"Failed to open settings file '{}' for reading", path.wstring());
        return false;
    }

    // Parse.
    auto json = nlohmann::json::parse(file, nullptr, false, true);
    if (json.is_discarded())
    {
        LOG_ERROR(L"Failed to parse settings '{}'", path.wstring());
        return false;
    }
    
    // Deserialize.
    try
    {
        json.get_to<Settings>(*this);
    }
    catch (nlohmann::json::exception& e)
    {
        LOG_ERROR(L"Failed to deserialize settings '{}'", path.wstring());
        LOG_DEBUG("what() {}", e.what());
        return false;
    }

    LOG_DEBUG("{}", json.dump(4));
    LOG_INFO(L"Loaded settings '{}'", path.wstring());

    return true;
#else
    return false;
#endif
}

auto Settings::Save (const fs::path& path) -> bool
{
#if defined(FEATURE_CAFFEINETAKE_SETTINGS)
    // Open Settings file for write.
    auto file = std::ofstream(path);
    if (!file)
    {
        LOG_ERROR(L"Failed to open settings file '{}' for writing", path.wstring());
        return false;
    }

    // Serialize.
    auto json = nlohmann::json(*this);
    file << std::setw(4) << json;

    LOG_INFO(L"Saved settings '{}'", path.wstring());

    return true;
#else
    return false;
#endif
}

} // namespace CaffeineTake
