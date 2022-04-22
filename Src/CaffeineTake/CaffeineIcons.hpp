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

#include <filesystem>
#include <string_view>

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
        Original,
        Square,
        Custom
    };

    enum class Theme : bool
    {
        Light,
        Dark
    };

private:
    HINSTANCE mInstanceHandle   = NULL;
    fs::path  mCustomIconsPath  = fs::path();

    inline auto LoadFromResource (int id, int w, int h) -> HICON;
    inline auto LoadFromFile     (std::wstring_view fileName, int w, int h) -> HICON;

    auto LoadOriginalIcons (Theme theme, int w, int h) -> bool;
    auto LoadSquareIcons   (Theme theme, int w, int h) -> bool;
    auto LoadCustomIcons   (Theme theme, int w, int h) -> bool;

    auto InternalCleanup () -> void;

public:
    HICON CaffeineDisabled     = NULL;
    HICON CaffeineEnabled      = NULL;
    HICON CaffeineAutoInactive = NULL;
    HICON CaffeineAutoActive   = NULL;

public:
    CaffeineIcons  (HINSTANCE hInstance);
    CaffeineIcons  (HINSTANCE hInstance, fs::path customIconsPath);
    ~CaffeineIcons ();

    auto Load (IconPack pack, Theme theme, int w, int h) -> bool;
};

} // namespace CaffeineTake
