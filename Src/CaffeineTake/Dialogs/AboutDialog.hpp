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

#include "Dialog.hpp"
#include "Resource.h"
#include "Version.hpp"

namespace CaffeineTake {

class AboutDialog : public Dialog<bool>
{
    bool m3rdParty;

    virtual auto OnInit    (HWND dlgHandle)               -> bool override;
    virtual auto OnOk      ()                             -> bool override { return true; }
    virtual auto OnCancel  ()                             -> bool override { return true; }
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnClose   ()                             -> void override {  }

public:
    AboutDialog ()
        : Dialog    (IDD_ABOUT)
        , m3rdParty (false)
    {
    }
};

auto AboutDialog::OnInit (HWND dlgHandle) -> bool
{
    const auto caffeineIcon = LoadImageW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCE(IDI_CAFFEINE_APP),
        IMAGE_ICON,
        48,
        48,
        LR_DEFAULTCOLOR | LR_SHARED
    );
    SendDlgItemMessage(mDlgHandle, IDC_ABOUT_CAFFEINE_LOGO, STM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(caffeineIcon));

    const auto name = std::wstring(CAFFEINE_TAKE_PROGRAM_NAME) + L" v" + std::wstring(CAFFEINE_TAKE_VERSION_STRING);
    SetDlgItemText(mDlgHandle, IDC_ABOUT_PROGRAM_NAME, name.c_str());
    SetDlgItemText(mDlgHandle, IDC_ABOUT_AUTHOR, CAFFEINE_TAKE_AUTHOR.data());
    SetDlgItemText(mDlgHandle, IDC_ABOUT_HOMEPAGE, CAFFEINE_TAKE_HOMEPAGE.data());
    SetDlgItemText(mDlgHandle, IDC_ABOUT_LICENSE, CAFFEINE_TAKE_LICENSE.data());

    return true;
}

auto AboutDialog::OnCommand (WPARAM wParam, LPARAM lParam) -> bool
{
    switch (LOWORD(wParam))
    {
    case IDC_ABOUT_BUTTON_3RDPARTY:
        if (m3rdParty)
        {
            SetDlgItemText(mDlgHandle, IDC_ABOUT_LICENSE, CAFFEINE_TAKE_LICENSE.data());
            SetDlgItemText(mDlgHandle, IDC_ABOUT_BUTTON_3RDPARTY, L"3rd Party Licenses");
        }
        else
        {
            SetDlgItemText(mDlgHandle, IDC_ABOUT_LICENSE, THIRDPARTY_LICENSES.data());
            SetDlgItemText(mDlgHandle, IDC_ABOUT_BUTTON_3RDPARTY, L"Program License");
        }

        m3rdParty = !m3rdParty;
        break;
    }

    return true;
}

auto AboutDialog::OnNotify (WPARAM wParam, LPARAM lParam) -> bool
{
    switch ((reinterpret_cast<LPNMHDR>(lParam))->code)
    {
    case NM_CLICK:
    case NM_RETURN:
    {
        auto link = reinterpret_cast<PNMLINK>(lParam);
        auto item = link->item;
        ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

        ResultCancel(); // close dialog
        break;
    }
    }

    return true;
}

} // namespace CaffeineTake
