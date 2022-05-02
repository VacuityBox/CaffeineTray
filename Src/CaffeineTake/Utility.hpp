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

#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using SystemTimePoint = std::chrono::system_clock::time_point;

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

auto GetDpi (HWND hWnd) -> int;

auto HexCharToInt (const char c) -> unsigned char;

SystemTimePoint FILETIME_to_system_clock (FILETIME fileTime);
FILETIME        system_clock_to_FILETIME (SystemTimePoint systemPoint);

#define SINGLE_INSTANCE_GUARD()                                                               \
    static std::mutex _mutex_##__LINE__;                                                      \
    auto _lock_##__LINE__ = std::unique_lock<std::mutex>(_mutex_##__LINE__, std::defer_lock); \
    if (!_lock_##__LINE__.try_lock()) { return false; }                                       \
    do {} while (0)

} // namespace CaffeineTake

// std::wstring serializer/deserializer.
namespace nlohmann {

template <>
struct adl_serializer<std::wstring>
{
    static void to_json(json& j, const std::wstring& opt)
    {
        auto utf8 = CaffeineTake::UTF16ToUTF8(opt.c_str());
        j = utf8 ? utf8.value() : "";
    }

    static void from_json(const json& j, std::wstring& opt)
    {
        auto utf16 = CaffeineTake::UTF8ToUTF16(j.get<std::string>());
        opt = utf16 ? utf16.value() : L"";
    }
};

} // namespace nlohmann