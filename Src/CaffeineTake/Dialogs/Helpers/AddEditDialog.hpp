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

#include "Dialogs/Dialog.hpp"
#include "Helpers/ItemList.hpp"
#include "Resource.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <vector>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <windowsx.h>

namespace CaffeineTake {

// Base class for AddDialog and EditDialog.
template <typename T>
class AddEditDialog : public Dialog<T>
{
protected:
    std::shared_ptr<ItemList> mItems;

private:
    //virtual auto OnInit    (HWND dlgHandle)               -> bool;
    //virtual auto OnOk      ()                             -> bool;
    virtual auto OnCancel  ()                             -> bool override { return true; }
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool override { return true; }
    virtual auto OnClose   ()                             -> void override { }

protected:
    auto GetCheckedRadio () -> ItemType;
    auto SetCheckedRadio (HWND dlgHandle, ItemType type) -> void;

    auto GetValueAndType     ()              -> std::optional<std::pair<std::wstring, ItemType>>;
    auto ShowErrorMessageBox (ItemType type) -> void;

public:
    AddEditDialog (std::shared_ptr<ItemList> items, std::wstring_view title)
        : Dialog<T> (IDD_ADD_EDIT, title)
        , mItems    (items)
    {
    }

    virtual ~AddEditDialog() {}
};

} // namespace CaffeineTake

// AddEditDialog definitions.
namespace CaffeineTake {

template <typename T>
auto AddEditDialog<T>::OnCommand (WPARAM wParam, LPARAM lParam) -> bool
{
    switch (LOWORD(wParam))
    {
    case IDC_EDITBOX_ADD_EDIT:
    {
        switch (HIWORD(wParam))
        {
            case EN_CHANGE:
            {
                // Enable/Disable OK button if text length > 0 and one of radio buttons is checked.
                auto editHandle = reinterpret_cast<HWND>(lParam);
                auto length     = Edit_GetTextLength(editHandle);
                auto buttonOk   = GetDlgItem(this->mDlgHandle, IDOK);
                if (length > 0)
                {
                    if (GetCheckedRadio() != ItemType::Invalid)
                    {
                        EnableWindow(buttonOk, TRUE);
                    }
                }
                else
                {
                    EnableWindow(buttonOk, FALSE);
                }
                break;
            }
        }
        break;
    }

    case IDC_RADIO_PROCESS_NAME:
    case IDC_RADIO_PROCESS_PATH:
    case IDC_RADIO_WINDOW_TITLE:
    {
        // Enable/Disable OK button if text length > 0.
        if (GetCheckedRadio() != ItemType::Invalid)
        {
            auto editHandle = GetDlgItem(this->mDlgHandle, IDC_EDITBOX_ADD_EDIT);
            auto length     = Edit_GetTextLength(editHandle);
            auto buttonOk   = GetDlgItem(this->mDlgHandle, IDOK);
            if (length > 0)
            {
                EnableWindow(buttonOk, TRUE);
            }
        }
        break;
    }
    }

    return true;
}

template <typename T>
auto AddEditDialog<T>::GetCheckedRadio () -> ItemType
{    
    if (Button_GetCheck(GetDlgItem(this->mDlgHandle, IDC_RADIO_PROCESS_NAME)) == BST_CHECKED)
    {
        return ItemType::Name;
    }
    if (Button_GetCheck(GetDlgItem(this->mDlgHandle, IDC_RADIO_PROCESS_PATH)) == BST_CHECKED)
    {
        return ItemType::Path;
    }
    if (Button_GetCheck(GetDlgItem(this->mDlgHandle, IDC_RADIO_WINDOW_TITLE)) == BST_CHECKED)
    {
        return ItemType::Window;
    }
    
    return ItemType::Invalid;
}

template <typename T>
auto AddEditDialog<T>::SetCheckedRadio (HWND dlgHandle, ItemType type) -> void
{
    switch (type)
    {
    case ItemType::Name:
        Button_SetCheck(GetDlgItem(dlgHandle, IDC_RADIO_PROCESS_NAME), BST_CHECKED);
        break;
    case ItemType::Path:
        Button_SetCheck(GetDlgItem(dlgHandle, IDC_RADIO_PROCESS_PATH), BST_CHECKED);
        break;
    case ItemType::Window:
        Button_SetCheck(GetDlgItem(dlgHandle, IDC_RADIO_WINDOW_TITLE), BST_CHECKED);
        break;
    }
}

template <typename T>
auto AddEditDialog<T>::GetValueAndType () -> std::optional<std::pair<std::wstring, ItemType>>
{
    // Get and store selected type checkbox.
    auto type = GetCheckedRadio();
    if (type == ItemType::Invalid)
    {
        return std::nullopt;
    }

    // Get and store edit control text.
    auto editControl = GetDlgItem(this->mDlgHandle, IDC_EDITBOX_ADD_EDIT);
    auto valueLength = Edit_GetTextLength(editControl);
    if (valueLength <= 0)
    {
        return std::nullopt;
    }

    auto value = std::wstring(static_cast<size_t>(valueLength) + 1, L'\0'); // make sure we have length + NUL
    Edit_GetText(editControl, value.data(), valueLength + 1);               // max size must include NUL character
    value.pop_back();                                                       // remove NUL character

    return std::make_pair(value, type);
}

template <typename T>
auto AddEditDialog<T>::ShowErrorMessageBox (ItemType type) -> void
{
    switch (type)
    {
    case ItemType::Name:
        MessageBox(
            this->mDlgHandle,
            L"Specified Process Name already exists!\nPlease use other name.",
            L"Error",
            MB_OK | MB_ICONEXCLAMATION
        );
        break;
    case ItemType::Path:
        MessageBox(
            this->mDlgHandle,
            L"Specified Process Path already exists!\nPlease use other name.",
            L"Error",
            MB_OK | MB_ICONEXCLAMATION
        );
        break;
    case ItemType::Window:
        MessageBox(
            this->mDlgHandle,
            L"Specified Window Title already exists!\nPlease use other name.",
            L"Error",
            MB_OK | MB_ICONEXCLAMATION
        );
        break;
    }
}

} // namespace CaffeineTake
