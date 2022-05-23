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
#include "Settings.hpp"

namespace {

// https://www.codeproject.com/Articles/2841/How-to-replace-a-color-in-a-HBITMAP
// by Dimitri Rochette
// Licence: Public Domain

#define COLORREF2RGB(Color) (Color & 0xff00) | ((Color >> 16) & 0xff) | ((Color << 16) & 0xff0000)

//-------------------------------------------------------------------------------
// ReplaceColor
//
// Author    : Dimitri Rochette drochette@coldcat.fr
// Specials Thanks to Joe Woodbury for his comments and code corrections
//
// Includes  : Only <windows.h>

//
// hBmp      : Source Bitmap
// cOldColor : Color to replace in hBmp
// cNewColor : Color used for replacement
// hBmpDC    : DC of hBmp ( default NULL ) could be NULL if hBmp is not selected
//
// Retcode   : HBITMAP of the modified bitmap or NULL for errors
//
//-------------------------------------------------------------------------------
HBITMAP ReplaceColor(HBITMAP hBmp, const CaffeineTake::CaffeineIcons::ColorMappings& mappings, HDC hBmpDC, int tolerance)
{
    HBITMAP RetBmp = NULL;
    if (hBmp)
    {
        HDC BufferDC = CreateCompatibleDC(NULL);    // DC for Source Bitmap
        if (BufferDC)
        {
            HBITMAP hTmpBitmap = (HBITMAP)NULL;
            if (hBmpDC)
            {
                if (hBmp == (HBITMAP)GetCurrentObject(hBmpDC, OBJ_BITMAP))
                {
                    hTmpBitmap = CreateBitmap(1, 1, 1, 1, NULL);
                    SelectObject(hBmpDC, hTmpBitmap);
                }
            }

            HGDIOBJ PreviousBufferObject = SelectObject(BufferDC, hBmp);
            // here BufferDC contains the bitmap

            HDC DirectDC = CreateCompatibleDC(NULL); // DC for working
            if (DirectDC)
            {
                // Get bitmap size
                BITMAP bm;
                GetObject(hBmp, sizeof(bm), &bm);

                // create a BITMAPINFO with minimal initilisation 
                // for the CreateDIBSection
                BITMAPINFO RGB32BitsBITMAPINFO;
                ZeroMemory(&RGB32BitsBITMAPINFO, sizeof(BITMAPINFO));
                RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                RGB32BitsBITMAPINFO.bmiHeader.biWidth = bm.bmWidth;
                RGB32BitsBITMAPINFO.bmiHeader.biHeight = bm.bmHeight;
                RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
                RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

                // pointer used for direct Bitmap pixels access
                UINT* ptPixels;

                HBITMAP DirectBitmap = CreateDIBSection(
                    DirectDC,
                    (BITMAPINFO*)&RGB32BitsBITMAPINFO,
                    DIB_RGB_COLORS,
                    (void**)&ptPixels,
                    NULL,
                    0
                );

                if (DirectBitmap)
                {
                    // here DirectBitmap!=NULL so ptPixels!=NULL no need to test
                    HGDIOBJ PreviousObject = SelectObject(DirectDC, DirectBitmap);
                    BitBlt(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, BufferDC, 0, 0, SRCCOPY);

                    // here the DirectDC contains the bitmap

                    // Convert COLORREF to RGB (Invert RED and BLUE)
                    //cOldColor = COLORREF2RGB(cOldColor);
                    //cNewColor = COLORREF2RGB(cNewColor);

                    // After all the inits we can do the job : Replace Color
                    for (int i = ((bm.bmWidth * bm.bmHeight) - 1); i >= 0; i--)
                    {
                        const auto IconRed   = int{(ptPixels[i] >> 16) & 0xFF};
                        const auto IconGreen = int{(ptPixels[i] >>  8) & 0xFF};
                        const auto IconBlue  = int{(ptPixels[i] >>  0) & 0xFF};
                        const auto IconAlpha = int{(ptPixels[i] >> 24) & 0xFF};

                        for (const auto& pair : mappings)
                        {
                            const auto OldRed   = int{(pair.first >> 16) & 0xFF};
                            const auto OldGreen = int{(pair.first >>  8) & 0xFF};
                            const auto OldBlue  = int{(pair.first >>  0) & 0xFF};
                            const auto OldAlpha = int{(pair.first >> 24) & 0xFF};

                            const auto dr = std::abs(IconRed   - OldRed);
                            const auto dg = std::abs(IconGreen - OldGreen);
                            const auto db = std::abs(IconBlue  - OldBlue);
                            const auto da = std::abs(IconAlpha - OldAlpha);

                            if (dr <= tolerance && dg <= tolerance && db <= tolerance)
                            {
                                const auto NewRed   = int{(pair.second >> 16) & 0xFF};
                                const auto NewGreen = int{(pair.second >>  8) & 0xFF};
                                const auto NewBlue  = int{(pair.second >>  0) & 0xFF};
                                const auto NewAlpha = int{(pair.second >> 24) & 0xFF};

                                if (NewAlpha == 0x00)
                                {
                                    ptPixels[i] = NewRed << 16 | NewGreen << 8 | NewBlue;
                                }
                                else
                                {
                                    ptPixels[i] = IconAlpha << 24 | NewRed << 16 | NewGreen << 8 | NewBlue;
                                }

                                break;
                            }
                        }
                    }

                    // little clean up
                    // Don't delete the result of SelectObject because it's 
                    // our modified bitmap (DirectBitmap)
                    SelectObject(DirectDC, PreviousObject);

                    // finish
                    RetBmp = DirectBitmap;
                }

                // clean up
                DeleteDC(DirectDC);
            }

            if (hTmpBitmap)
            {
                SelectObject(hBmpDC, hBmp);
                DeleteObject(hTmpBitmap);
            }

            SelectObject(BufferDC, PreviousBufferObject);

            // BufferDC is now useless
            DeleteDC(BufferDC);
        }
    }
    return RetBmp;
}

} // anonymous namespace

namespace CaffeineTake {

#define DESTROY_ICON(_icon) if (_icon) { DestroyIcon(_icon); _icon = NULL; }
#define PREP_COLORS(_b, _list, _state, _mode) (_b) ? PrepareColors(_list, _state, _mode) : _list

    
inline auto CaffeineIcons::InternalIconThemeToString (InternalIconTheme theme) -> std::wstring_view
{
    switch (theme)
    {
    case CaffeineTake::CaffeineIcons::InternalIconTheme::Light:
        return L"Light";
    case CaffeineTake::CaffeineIcons::InternalIconTheme::Dark:
        return L"Dark";
    case CaffeineTake::CaffeineIcons::InternalIconTheme::Custom:
        return L"Custom";
    }

    return L"Invalid InternalIconTheme";
}

auto CaffeineIcons::ReplaceColors (HICON icon, const IconColors& colors) -> HICON
{
    auto updatedIcon = HICON{icon};

    auto info = ICONINFO{};
    if (!GetIconInfo(icon, &info))
    {
        LOG_ERROR("GetIconInfo() failed with code {}", GetLastError());
    }
    else
    {
        auto mappings = ColorMappings{};
        
        mappings.push_back(std::make_pair(COLOR_MAPPING_BORDER, colors.CupBorder));      // border
        mappings.push_back(std::make_pair(COLOR_MAPPING_MODE  , colors.ModeIndicator));  // letter
        mappings.push_back(std::make_pair(COLOR_MAPPING_FILL  , colors.CupFill));        // fill
        mappings.push_back(std::make_pair(COLOR_MAPPING_STEAM , colors.Steam));          // steam

        // Replace colors.
        auto colorBitmap = ReplaceColor(info.hbmColor, mappings, NULL, 16);
        //auto alphaBitmap = ReplaceColor(info.hbmMask, mappings, NULL, 16);

        // Update color bitmap.
        DeleteBitmap(info.hbmColor);
        //DeleteBitmap(info.hbmMask);
        info.hbmColor = colorBitmap;
        //info.hbmMask  = alphaBitmap;

        // Create new icon.
        updatedIcon = CreateIconIndirect(&info);

        // Cleanup.
        DeleteBitmap(info.hbmColor);
        DeleteBitmap(info.hbmMask);
    }

    return updatedIcon;
}

auto CaffeineIcons::PrepareColors (const IconColors& colors, CaffeineState state, bool indicator) -> IconColors
{
    auto cooked = IconColors(colors);
    
    if (state == CaffeineState::Inactive)
    {
        cooked.CupFill = colors.CupFill & 0x00FFFFFF;
        cooked.Steam   = colors.Steam & 0x00FFFFFF;
    }

    if (!indicator)
    {
        cooked.ModeIndicator = colors.ModeIndicator & 0x00FFFFFF;
    }

    return cooked;
}

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

auto CaffeineIcons::LoadOriginalIcons (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool
{
    LOG_INFO(L"Loading Original icons (theme: {} [{}x{}])...", InternalIconThemeToString(theme), w, h);
    switch (theme)
    {
    case InternalIconTheme::Light:
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        // TODO change when timer icons added
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        break;
    case InternalIconTheme::Dark:
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_ORIGINAL_CAFFEINE_DISABLED_DARK, w, h);
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

auto CaffeineIcons::LoadSquareIcons (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool
{
    LOG_INFO(L"Loading Square icons (theme: {} [{}x{}])...", InternalIconThemeToString(theme), w, h);
    switch (theme)
    {
    case InternalIconTheme::Light:
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_LIGHT, w, h);
        CaffeineStandardActive   = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_ENABLED_LIGHT, w, h);
        CaffeineAutoInactive     = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_INACTIVE_LIGHT, w, h);
        CaffeineAutoActive       = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_AUTO_ACTIVE_LIGHT, w, h);
        CaffeineTimerInactive    = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_INACTIVE_LIGHT, w, h);
        CaffeineTimerActive      = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_TIMER_ACTIVE_LIGHT, w, h);
        break;
    case InternalIconTheme::Dark:
        CaffeineStandardInactive = LoadFromResource(IDI_NOTIFY_SQUARE_CAFFEINE_DISABLED_DARK, w, h);
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

auto CaffeineIcons::LoadRoundIcons (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool
{
    LOG_INFO(L"Loading Round icons (theme: {} [{}x{}])...", InternalIconThemeToString(theme), w, h);

    auto StandardModeIcon = LoadFromResource(IDI_NOTIFY_ROUND_CAFFEINE_STANDARD_MODE, w, h);
    auto AutoModeIcon     = LoadFromResource(IDI_NOTIFY_ROUND_CAFFEINE_AUTO_MODE, w, h);
    auto TimerModeIcon    = LoadFromResource(IDI_NOTIFY_ROUND_CAFFEINE_TIMER_MODE, w, h);
    
    auto& prep = settings->General.PrepareIconColors;
    auto& icl  = settings->General.IconColors;

    LOG_INFO("Finished loading icons");

    switch (theme)
    {
    case InternalIconTheme::Light:
        CaffeineStandardInactive = ReplaceColors(StandardModeIcon , PrepareColors(mLightThemeColors, CaffeineState::Inactive, false));
        CaffeineStandardActive   = ReplaceColors(StandardModeIcon , PrepareColors(mLightThemeColors, CaffeineState::Active, false));
        CaffeineAutoInactive     = ReplaceColors(AutoModeIcon     , PrepareColors(mLightThemeColors, CaffeineState::Inactive, true));
        CaffeineAutoActive       = ReplaceColors(AutoModeIcon     , PrepareColors(mLightThemeColors, CaffeineState::Active, true));
        CaffeineTimerInactive    = ReplaceColors(TimerModeIcon    , PrepareColors(mLightThemeColors, CaffeineState::Inactive, true));
        CaffeineTimerActive      = ReplaceColors(TimerModeIcon    , PrepareColors(mLightThemeColors, CaffeineState::Active, true));
        break;
    case InternalIconTheme::Dark:
        CaffeineStandardInactive = ReplaceColors(StandardModeIcon , PrepareColors(mDarkThemeColors, CaffeineState::Inactive, false));
        CaffeineStandardActive   = ReplaceColors(StandardModeIcon , PrepareColors(mDarkThemeColors, CaffeineState::Active, false));
        CaffeineAutoInactive     = ReplaceColors(AutoModeIcon     , PrepareColors(mDarkThemeColors, CaffeineState::Inactive, true));
        CaffeineAutoActive       = ReplaceColors(AutoModeIcon     , PrepareColors(mDarkThemeColors, CaffeineState::Active, true));
        CaffeineTimerInactive    = ReplaceColors(TimerModeIcon    , PrepareColors(mDarkThemeColors, CaffeineState::Inactive, true));
        CaffeineTimerActive      = ReplaceColors(TimerModeIcon    , PrepareColors(mDarkThemeColors, CaffeineState::Active, true));
        break;
    case InternalIconTheme::Custom:        
        CaffeineStandardInactive = ReplaceColors(StandardModeIcon , PREP_COLORS(prep, icl.StandardMode_Inactive, CaffeineState::Inactive, false));
        CaffeineStandardActive   = ReplaceColors(StandardModeIcon , PREP_COLORS(prep, icl.StandardMode_Active  , CaffeineState::Active  , false));
        CaffeineAutoInactive     = ReplaceColors(AutoModeIcon     , PREP_COLORS(prep, icl.AutoMode_Inactive    , CaffeineState::Inactive, true));
        CaffeineAutoActive       = ReplaceColors(AutoModeIcon     , PREP_COLORS(prep, icl.AutoMode_Active      , CaffeineState::Active  , true));
        CaffeineTimerInactive    = ReplaceColors(TimerModeIcon    , PREP_COLORS(prep, icl.TimerMode_Inactive   , CaffeineState::Inactive, true));
        CaffeineTimerActive      = ReplaceColors(TimerModeIcon    , PREP_COLORS(prep, icl.TimerMode_Active     , CaffeineState::Active  , true));
        break;
    }

    DESTROY_ICON(StandardModeIcon);
    DESTROY_ICON(AutoModeIcon);
    DESTROY_ICON(TimerModeIcon);

    return true;
}

auto CaffeineIcons::LoadCustomIcons (InternalIconTheme theme, int w, int h, SettingsPtr settings) -> bool
{
    LOG_INFO(L"Loading Custom icons (theme: {} [{}x{}])...", InternalIconThemeToString(theme), w, h);
    switch (theme)
    {
    case InternalIconTheme::Light:        
        CaffeineStandardInactive = LoadFromFile(L"CaffeineDisabledLight.ico", w, h);
        CaffeineStandardActive   = LoadFromFile(L"CaffeineEnabledLight.ico", w, h);
        CaffeineAutoInactive     = LoadFromFile(L"CaffeineAutoInactiveLight.ico", w, h);
        CaffeineAutoActive       = LoadFromFile(L"CaffeineAutoActiveLight.ico", w, h);
        CaffeineTimerInactive    = LoadFromFile(L"CaffeineTimerInactiveLight.ico", w, h);
        CaffeineTimerActive      = LoadFromFile(L"CaffeineTimerActiveLight.ico", w, h);
        break;
    case InternalIconTheme::Dark:
        CaffeineStandardInactive = LoadFromFile(L"CaffeineDisabledDark.ico", w, h);
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

auto CaffeineIcons::InternalCleanup () -> void
{
    LOG_INFO("Cleaning up icons...");
    
    DESTROY_ICON(CaffeineStandardInactive);
    DESTROY_ICON(CaffeineStandardActive  );
    DESTROY_ICON(CaffeineAutoInactive    );
    DESTROY_ICON(CaffeineAutoActive      );
    DESTROY_ICON(CaffeineTimerInactive   );
    DESTROY_ICON(CaffeineTimerActive     );

    LOG_INFO("Finished cleaning-up icons");
}

CaffeineIcons::CaffeineIcons (HINSTANCE hInstance, fs::path customIconsPath)
    : mInstanceHandle  (hInstance)
    , mCustomIconsPath (customIconsPath)
{
    {
        mLightThemeColors.CupBorder     = 0xFFFFFFFF;
        mLightThemeColors.CupFill       = 0xFFFFFFFF;
        mLightThemeColors.Steam         = 0xFFFFFFFF;
        mLightThemeColors.ModeIndicator = 0xFFFFFFFF;
    }
    {
        mDarkThemeColors.CupBorder     = 0xFF000000;
        mDarkThemeColors.CupFill       = 0xFF000000;
        mDarkThemeColors.Steam         = 0xFF000000;
        mDarkThemeColors.ModeIndicator = 0xFF000000;
    }
}

CaffeineIcons::~CaffeineIcons ()
{
    InternalCleanup();
}

auto CaffeineIcons::Load (IconPack pack, SystemTheme theme, int w, int h, SettingsPtr settings) -> bool
{
    InternalCleanup();

    auto iconTheme = InternalIconTheme::Custom;
    if (settings->General.IconTheme == IconTheme::System)
    {
        switch (theme)
        {
        case SystemTheme::Light:
            iconTheme = InternalIconTheme::Light;
            break;
        case SystemTheme::Dark:
            iconTheme = InternalIconTheme::Dark;
            break;
        }
    }

    switch (pack)
    {
    case CaffeineTake::CaffeineIcons::IconPack::Original:
        return LoadOriginalIcons(iconTheme, w, h, settings);
    case CaffeineTake::CaffeineIcons::IconPack::Square:
        return LoadSquareIcons(iconTheme, w, h, settings);
    case CaffeineTake::CaffeineIcons::IconPack::Round:
        return LoadRoundIcons(iconTheme, w, h, settings);
    case CaffeineTake::CaffeineIcons::IconPack::Custom:
        return LoadCustomIcons(iconTheme, w, h, settings);
    }

    return false;
}

} // namespace CaffeineTake
