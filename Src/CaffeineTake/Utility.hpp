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

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

enum class SessionState : bool
{
    Unlocked,
    Locked
};

auto UTF8ToUTF16 (const std::string_view str) -> std::optional<std::wstring>;
auto UTF16ToUTF8 (const std::wstring_view str) -> std::optional<std::string>;

auto GetAppDataPath  () -> std::filesystem::path;
auto IsSessionLocked () -> SessionState;

auto ScanProcesses  (std::function<bool (HANDLE, DWORD, const std::wstring_view)> checkFn) -> bool;
auto ScanWindows    (std::function<bool (HWND, DWORD, const std::wstring_view)> checkFn, bool onlyVisible = true) -> bool;
auto GetProcessPath (DWORD pid) -> std::filesystem::path;

#define SINGLE_INSTANCE_GUARD()                                                               \
    static std::mutex _mutex_##__LINE__;                                                      \
    auto _lock_##__LINE__ = std::unique_lock<std::mutex>(_mutex_##__LINE__, std::defer_lock); \
    if (!_lock_##__LINE__.try_lock()) { return false; }                                       \
    do {} while (0)

} // namespace CaffeineTake
