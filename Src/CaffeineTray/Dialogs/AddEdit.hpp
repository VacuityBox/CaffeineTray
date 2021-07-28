#pragma once

#include "Dialog.hpp"
#include "Helpers/AddDialogResult.hpp"
#include "Helpers/Item.hpp"
#include "Settings.hpp"
#include "resource.h"

#include <filesystem>
#include <map>
#include <memory>
#include <vector>
#include <utility>

#include "Platform.hpp"

namespace Caffeine {

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

} // namespace Caffeine

namespace Caffeine {

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
    auto radioProcessName = GetDlgItem(this->mDlgHandle, IDC_RADIO_PROCESS_NAME);
    auto radioProcessPath = GetDlgItem(this->mDlgHandle, IDC_RADIO_PROCESS_PATH);
    auto radioWindowTitle = GetDlgItem(this->mDlgHandle, IDC_RADIO_WINDOW_TITLE);

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
    Edit_GetText(editControl, value.data(), valueLength + 1);        // max size must include NUL character
    value.pop_back();                                                // remove NUL character

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

} // namespace Caffeine
