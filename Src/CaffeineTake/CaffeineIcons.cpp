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
#include "CaffeineIcons.hpp"

#include "Logger.hpp"
#include "Resource.hpp"

namespace CaffeineTake {

inline auto CaffeineIcons::LoadFromResource (int id, int w, int h) -> HICON
{
    const auto flags = UINT{LR_DEFAULTCOLOR | LR_SHARED};

    auto ico = static_cast<HICON>(LoadImageW(mInstanceHandle, MAKEINTRESOURCEW(id), IMAGE_ICON, w, h, flags));
    if (!ico)
    {
        LOG_ERROR("Failed to load icon: {}", id);
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
        LOG_ERROR("Failed to load icon: '{}'", path.string());
    }

    return ico;
}

auto CaffeineIcons::LoadOriginalIcons (Theme theme, int w, int h) -> bool
{
    LOG_INFO("Loading Original icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabledInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineDisabledActive   = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        // TODO change when timer icons added
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabledInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_DARK, w, h);
        CaffeineDisabledActive   = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_DARK, w, h);
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_DARK, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_DARK, w, h);
        // TODO change when timer icons added
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_DARK, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_DARK, w, h);
        break;
    }
    LOG_INFO("Finished loading icons");

    return true;
}

auto CaffeineIcons::LoadSquareIcons (Theme theme, int w, int h) -> bool
{
    LOG_INFO("Loading Square icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabledInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineDisabledActive   = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_INACTIVE_LIGHT, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_ACTIVE_LIGHT, w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabledInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_DARK, w, h);
        CaffeineDisabledActive   = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_DARK, w, h);;
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_DARK, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_DARK, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_DARK, w, h);
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_INACTIVE_DARK, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_ACTIVE_DARK, w, h);
        break;
    }
    LOG_INFO("Finished loading icons");

    return true;
}

auto CaffeineIcons::LoadCustomIcons (Theme theme, int w, int h) -> bool
{
    LOG_INFO("Loading Custom icons (theme: {} [{}x{}])...", theme == Theme::Light ? "light" : "dark", w, h);
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::Theme::Light:
        CaffeineDisabledInactive = LoadFromFile(L"CaffeineDisabledLight.ico", w, h);
        CaffeineDisabledActive   = LoadFromFile(L"CaffeineDisabledLight.ico", w, h);
        CaffeineStandardInactive = LoadFromFile(L"CaffeineEnabledLight.ico", w, h);
        CaffeineStandardActive   = LoadFromFile(L"CaffeineEnabledLight.ico", w, h);
        CaffeineAutoInactive     = LoadFromFile(L"CaffeineAutoInactiveLight.ico", w, h);
        CaffeineAutoActive       = LoadFromFile(L"CaffeineAutoActiveLight.ico", w, h);
        CaffeineTimerInactive    = LoadFromFile(L"CaffeineTimerInactiveLight.ico", w, h);
        CaffeineTimerActive      = LoadFromFile(L"CaffeineTimerActiveLight.ico", w, h);
        break;
    case CaffeineTake::CaffeineIcons::Theme::Dark:
        CaffeineDisabledInactive = LoadFromFile(L"CaffeineDisabledDark.ico", w, h);
        CaffeineDisabledActive   = LoadFromFile(L"CaffeineDisabledDark.ico", w, h);
        CaffeineStandardInactive = LoadFromFile(L"CaffeineEnabledDark.ico", w, h);
        CaffeineStandardActive   = LoadFromFile(L"CaffeineEnabledDark.ico", w, h);
        CaffeineAutoInactive     = LoadFromFile(L"CaffeineAutoInactiveDark.ico", w, h);
        CaffeineAutoActive       = LoadFromFile(L"CaffeineAutoActiveDark.ico", w, h);
        CaffeineTimerInactive    = LoadFromFile(L"CaffeineTimerInactiveDark.ico", w, h);
        CaffeineTimerActive      = LoadFromFile(L"CaffeineTimerActiveDark.ico", w, h);
        break;
    }
    LOG_INFO("Finished loading icons");

    return true;
}

#define DESTROY_ICON(_icon) if (_icon) { DestroyIcon(_icon); _icon = NULL; }

auto CaffeineIcons::InternalCleanup () -> void
{
    LOG_INFO("Cleaning up icons...");
    
    DESTROY_ICON(CaffeineDisabledInactive);
    DESTROY_ICON(CaffeineDisabledActive  );
    DESTROY_ICON(CaffeineStandardInactive);
    DESTROY_ICON(CaffeineStandardActive  );
    DESTROY_ICON(CaffeineAutoInactive    );
    DESTROY_ICON(CaffeineAutoActive      );
    DESTROY_ICON(CaffeineTimerInactive   );
    DESTROY_ICON(CaffeineTimerActive     );

    LOG_INFO("Finished cleaning-up icons");
}

#undef DESTROY_ICON

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
