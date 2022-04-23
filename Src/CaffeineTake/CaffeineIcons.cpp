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

#include "CaffeineIcons.hpp"

#include "resource.h"

#include <spdlog/spdlog.h>

namespace CaffeineTake {

inline auto CaffeineIcons::LoadFromResource (int id, int w, int h) -> HICON
{
    const auto flags = UINT{LR_DEFAULTCOLOR | LR_SHARED};

    auto ico = static_cast<HICON>(LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(id), IMAGE_ICON, w, h, flags));
    if (!ico)
    {
        spdlog::error("Failed to load icon: {}", id);
    }

    return ico;
}

inline auto CaffeineIcons::LoadFromFile (std::wstring_view fileName, int w, int h) -> HICON
{
    const auto flags = UINT{LR_DEFAULTCOLOR | LR_LOADFROMFILE};
    const auto path  = mCustomIconsPath / fileName;
    const auto str   = path.wstring();

    auto ico = static_cast<HICON>(LoadImageW(mInstanceHandle, str.c_str(), IMAGE_ICON, w, h, flags));
    if (!ico)
    {
        spdlog::error("Failed to load icon: '{}'", path.string());
    }

    return ico;
}

auto CaffeineIcons::LoadOriginalIcons (Theme theme, int w, int h) -> bool
{
    spdlog::info("Loading Original icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabled     = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineEnabled      = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabled     = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_DARK, w, h);
        CaffeineEnabled      = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_DARK, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_DARK, w, h);
        break;
    }
    spdlog::info("Finished loading icons");

    return CaffeineDisabled && CaffeineEnabled && CaffeineAutoInactive && CaffeineAutoActive;
}

auto CaffeineIcons::LoadSquareIcons (Theme theme, int w, int h) -> bool
{
    spdlog::info("Loading Square icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabled     = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineEnabled      = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabled     = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_DARK, w, h);
        CaffeineEnabled      = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_DARK, w, h);
        CaffeineAutoInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_DARK, w, h);
        break;
    }
    spdlog::info("Finished loading icons");

    return CaffeineDisabled && CaffeineEnabled && CaffeineAutoInactive && CaffeineAutoActive;
}

auto CaffeineIcons::LoadCustomIcons (Theme theme, int w, int h) -> bool
{
    spdlog::info("Loading Custom icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabled     = LoadFromFile(L"CaffeineDisabledLight.ico", w, h);
        CaffeineEnabled      = LoadFromFile(L"CaffeineEnabledLight.ico", w, h);
        CaffeineAutoInactive = LoadFromFile(L"CaffeineAutoInactiveLight.ico", w, h);
        CaffeineAutoInactive = LoadFromFile(L"CaffeineAutoActiveLight.ico", w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabled     = LoadFromFile(L"CaffeineDisabledDark.ico", w, h);
        CaffeineEnabled      = LoadFromFile(L"CaffeineEnabledDark.ico", w, h);
        CaffeineAutoInactive = LoadFromFile(L"CaffeineAutoInactiveDark.ico", w, h);
        CaffeineAutoInactive = LoadFromFile(L"CaffeineAutoActiveDark.ico", w, h);
        break;
    }
    spdlog::info("Finished loading icons");

    return CaffeineDisabled && CaffeineEnabled && CaffeineAutoInactive && CaffeineAutoActive;
}

auto CaffeineIcons::InternalCleanup () -> void
{
    spdlog::info("Cleaning up icons...");
    if (CaffeineDisabled)
    {
        DestroyIcon(CaffeineDisabled);
        CaffeineDisabled = NULL;
    }
    if (CaffeineEnabled)
    {
        DestroyIcon(CaffeineEnabled);
        CaffeineEnabled = NULL;
    }
    if (CaffeineAutoInactive)
    {
        DestroyIcon(CaffeineAutoInactive);
        CaffeineAutoInactive = NULL;
    }
    if (CaffeineAutoActive)
    {
        DestroyIcon(CaffeineAutoActive);
        CaffeineAutoActive = NULL;
    }
    spdlog::info("Finished cleaning-up icons");
}

CaffeineIcons::CaffeineIcons (HINSTANCE hInstance)
    : mInstanceHandle  (hInstance)
{
}

CaffeineIcons::CaffeineIcons (HINSTANCE hInstance, fs::path customIconsPath)
    : mInstanceHandle  (hInstance)
    , mCustomIconsPath (customIconsPath)
{
}

CaffeineIcons::~CaffeineIcons ()
{
    InternalCleanup();
}

auto CaffeineIcons::Load (IconPack pack, Theme theme, int w, int h) -> bool
{
    InternalCleanup();

    switch (pack)
    {
    case CaffeineTake::CaffeineIcons::IconPack::Original:
        return LoadOriginalIcons(theme, w, h);
    case CaffeineTake::CaffeineIcons::IconPack::Square:
        return LoadSquareIcons(theme, w, h);
    case CaffeineTake::CaffeineIcons::IconPack::Custom:
        return LoadCustomIcons(theme, w, h);
    }

    return false;
}

} // namespace CaffeineTake
