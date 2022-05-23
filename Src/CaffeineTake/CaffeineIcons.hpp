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

#include "CaffeineState.hpp"
#include "ForwardDeclaration.hpp"

#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class CaffeineIcons
{
public:
    enum class IconPack : unsigned char
    {
        Original = 0,
        Square   = 1,
        Round    = 2,
        Custom   = 3
    };

    enum class IconTheme : unsigned char
    {
        System = 0, // auto
        Custom = 1,
    };

    enum class SystemTheme : bool
    {
        Light  = 0,
        Dark   = 1
    };

    using Color = uint32_t; // 0xAARRGGBB
    using ColorMappings = std::vector<std::pair<Color, Color>>;

    struct IconColors
    {
        Color CupBorder     = 0xFFFFFFFF;
        Color CupFill       = 0xFFFFFFFF;
        Color Steam         = 0xFFFFFFFF;
        Color ModeIndicator = 0xFFFFFFFF;

        IconColors () = default;
    };

private:
    static constexpr auto COLOR_MAPPING_BORDER = Color(0xffff00ff);
    static constexpr auto COLOR_MAPPING_MODE   = Color(0xff00ff00);
    static constexpr auto COLOR_MAPPING_FILL   = Color(0xffffff00);
    static constexpr auto COLOR_MAPPING_STEAM  = Color(0xff00ffff);

    enum class InternalIconTheme
    {
        Light  = 0,
        Dark   = 1,
        Custom = 2
    };

    inline auto InternalIconThemeToString (InternalIconTheme theme) -> std::wstring_view;

private:
    HINSTANCE  mInstanceHandle   = NULL;
    fs::path   mCustomIconsPath  = fs::path();
    IconColors mLightThemeColors = IconColors();
    IconColors mDarkThemeColors  = IconColors();

    auto ReplaceColors (HICON icon, const IconColors& colors) -> HICON;
    auto PrepareColors (const IconColors& colors, CaffeineState state, bool indicator) -> IconColors;

    inline auto LoadFromResource (int id, int w, int h) -> HICON;
    inline auto LoadFromFile     (std::wstring_view fileName, int w, int h) -> HICON;

    auto LoadOriginalIcons (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool;
    auto LoadSquareIcons   (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool;
    auto LoadRoundIcons    (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool;
    auto LoadCustomIcons   (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool;

    auto InternalCleanup () -> void;

public:
    HICON CaffeineStandardInactive  = NULL;
    HICON CaffeineStandardActive    = NULL;
    HICON CaffeineAutoInactive      = NULL;
    HICON CaffeineAutoActive        = NULL;
    HICON CaffeineTimerInactive     = NULL;
    HICON CaffeineTimerActive       = NULL;

public:
    CaffeineIcons  (HINSTANCE hInstance, fs::path customIconsPath);
    ~CaffeineIcons ();

    auto Load (IconPack pack, SystemTheme theme, int w, int h, SettingsPtr settings) -> bool;
};

} // namespace CaffeineTake
