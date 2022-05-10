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
#include "AddWizard.hpp"

#include "Helpers/ErrorMessages.hpp"

namespace CaffeineTake {

auto AddWizardDialog::OnInit (HWND dlgHandle) -> bool
{
    // Create list view columns.
    auto listView = GetDlgItem(dlgHandle, IDC_LISTVIEW_WIZARD_PROCESSES_AND_WINDOWS);
    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT);

    auto rect = RECT{};
    GetWindowRect(listView, &rect);

    auto lvc     = LVCOLUMN{ 0 };
    lvc.mask     = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.iSubItem = 0;
    lvc.cx       = rect.right - rect.left - 40;
    lvc.pszText  = const_cast<LPWSTR>(L"Process");
    ListView_InsertColumn(listView, 0, &lvc);
    //ListView_SetColumnWidth(listView, 0, LVSCW_AUTOSIZE);

    // Deactivate Add buttons.
    EnableWindow(GetDlgItem(dlgHandle, IDC_BUTTON_WIZARD_ADD_NAME), FALSE);
    EnableWindow(GetDlgItem(dlgHandle, IDC_BUTTON_WIZARD_ADD_PATH), FALSE);
    EnableWindow(GetDlgItem(dlgHandle, IDC_BUTTON_WIZARD_ADD_WINDOW), FALSE);

    // Fill the list with running processes.
    ListView_SetImageList(listView, mIconCache->GetImageList(), LVSIL_SMALL);

    Refresh(listView);

    return true;
}

auto AddWizardDialog::OnOk () -> bool
{
    return true;
}

auto AddWizardDialog::OnCancel () -> bool
{
    return true;
}

auto AddWizardDialog::OnCommand (WPARAM wParam, LPARAM lParam) -> bool
{
    auto enableButtonIfEditboxLength = [&](int buttonId)
    {
        // Enable/Disable OK button if text length > 0.
        auto editHandle = reinterpret_cast<HWND>(lParam);
        auto length     = Edit_GetTextLength(editHandle);
        auto button     = GetDlgItem(mDlgHandle, buttonId);
        EnableWindow(button, length > 0);
    };

    auto addButtonClick = [&](int buttonId, std::wstring_view error) -> bool
    {
        // Get selected;
        auto listView      = GetDlgItem(mDlgHandle, IDC_LISTVIEW_WIZARD_PROCESSES_AND_WINDOWS);
        auto selectedIndex = ListView_GetSelectionMark(listView);
        if (selectedIndex == -1)
        {
            return false;
        }
        
        const auto& processList = mRunningProcesses->Get();
        if (selectedIndex >= processList.size())
        {
            return false;
        }

        const auto& [pid, process] = processList[selectedIndex];

        // Check if not exist.
        const auto& value = L"";
        switch (buttonId)
        {
        case IDC_BUTTON_WIZARD_ADD_NAME:
            if (!mItems->Exists(process.name, ItemType::Name))
            {
                mResult.value = process.name;
                mResult.type  = ItemType::Name;
                mResult.icon  = process.icon;
                ResultOk();
                return true;
            }
            break;
        case IDC_BUTTON_WIZARD_ADD_PATH:
            if (!mItems->Exists(process.path, ItemType::Path))
            {
                mResult.value = process.path;
                mResult.type  = ItemType::Path;
                mResult.icon  = process.icon;
                ResultOk();
                return true;
            }
            break;
        case IDC_BUTTON_WIZARD_ADD_WINDOW:
            if (!mItems->Exists(process.window, ItemType::Window))
            {
                mResult.value = process.window;
                mResult.type  = ItemType::Window;
                mResult.icon  = process.icon;
                ResultOk();
                return true;
            }
            break;
        }
        
        MessageBox(mDlgHandle, error.data(), L"Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    };

    switch (LOWORD(wParam))
    {
    case IDC_EDITBOX_WIZARD_PROCESS_NAME:
    {
        switch (HIWORD(wParam))
        {
            case EN_CHANGE:
            {
                enableButtonIfEditboxLength(IDC_BUTTON_WIZARD_ADD_NAME);
                break;
            }
        }
        break;
    }
    case IDC_EDITBOX_WIZARD_PROCESS_PATH:
    {
        switch (HIWORD(wParam))
        {
            case EN_CHANGE:
            {
                enableButtonIfEditboxLength(IDC_BUTTON_WIZARD_ADD_PATH);
                break;
            }
        }
        break;
    }
    case IDC_EDITBOX_WIZARD_WINDOW_TITLE:
    {
        switch (HIWORD(wParam))
        {
            case EN_CHANGE:
            {
                enableButtonIfEditboxLength(IDC_BUTTON_WIZARD_ADD_WINDOW);
                break;
            }
        }
        break;
    }

    case IDC_BUTTON_WIZARD_REFRESH:
    {
        auto listView = GetDlgItem(mDlgHandle, IDC_LISTVIEW_WIZARD_PROCESSES_AND_WINDOWS);
        Refresh(listView, true);
        break;
    }

    case IDC_BUTTON_WIZARD_ADD_NAME:
    {
        addButtonClick(IDC_BUTTON_WIZARD_ADD_NAME, ERROR_NAME_ALREADY_EXISTS);
        break;
    }

    case IDC_BUTTON_WIZARD_ADD_PATH:
    {
        addButtonClick(IDC_BUTTON_WIZARD_ADD_PATH, ERROR_PATH_ALREADY_EXISTS);
        break;
    }

    case IDC_BUTTON_WIZARD_ADD_WINDOW:
    {
        addButtonClick(IDC_BUTTON_WIZARD_ADD_WINDOW, ERROR_WINDOW_ALREADY_EXISTS);
        break;
    }

    }

    return true;
}

auto AddWizardDialog::OnNotify (WPARAM wParam, LPARAM lParam) -> bool
{
    auto lpnmh    = reinterpret_cast<LPNMHDR>(lParam);
    auto listView = GetDlgItem(mDlgHandle, IDC_LISTVIEW_PROCESSES_AND_WINDOWS);

    switch (LOWORD(wParam))
    {

    case IDC_LISTVIEW_WIZARD_PROCESSES_AND_WINDOWS:
    {
    switch (lpnmh->code)
    {
        case LVN_GETDISPINFO:
        {
            auto nmlvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);    

            if (nmlvdi->item.iItem == -1)
            {
                break;
            }

            if (nmlvdi->item.mask & LVIF_TEXT)
            {
                switch (nmlvdi->item.iSubItem)
                {
                case 0:
                {
                    auto& processList    = mRunningProcesses->Get();
                    auto& [pid, process] = processList[nmlvdi->item.iItem];

                    // Use process name for text.
                    nmlvdi->item.pszText = process.name.data();
                    
                    if (process.icon != INVALID_ICON_ID)
                    {
                        nmlvdi->item.iImage = process.icon;
                    }
                    break;
                }
                default:
                    break;
                }
            }

            break;
        }

        case LVN_ITEMCHANGED:
        {
            auto lpnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
            if (lpnmia->iItem != -1)
            {
                auto editboxName   = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_PROCESS_NAME);
                auto editboxPath   = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_PROCESS_PATH);
                auto editboxWindow = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_WINDOW_TITLE);

                auto& processList    = mRunningProcesses->Get();
                auto& [pid, process] = processList[lpnmia->iItem]; 

                Edit_SetText(editboxName, process.name.data());
                Edit_SetText(editboxPath, process.path.data());
                Edit_SetText(editboxWindow, process.window.data());
            }
            break;
        }
    }
    }

    } // switch (LOWORD(wParam))

    return true;
}

auto AddWizardDialog::OnClose () -> void
{
}

auto AddWizardDialog::Refresh (HWND listView, bool select) -> void
{
    ListView_DeleteAllItems(listView);
    
    mRunningProcesses->Refresh();
    const auto& processList = mRunningProcesses->Get();

    // Update listview.
    ListView_SetItemCountEx(listView, processList.size(), LVSICF_NOSCROLL);

    if (select)
    {
        if (processList.size() > 0)
        {
            ListView_SetSelectionMark(listView, 0);
            ListView_SetItemState(listView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(listView, 0, false);
            SetFocus(listView);
        }
        else
        {
            auto editboxName   = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_PROCESS_NAME);
            auto editboxPath   = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_PROCESS_PATH);
            auto editboxWindow = GetDlgItem(mDlgHandle, IDC_EDITBOX_WIZARD_WINDOW_TITLE);

            Edit_SetText(editboxName, L"");
            Edit_SetText(editboxPath, L"");
            Edit_SetText(editboxWindow, L"");
        }
    }
}

} // namespace CaffeineTake
