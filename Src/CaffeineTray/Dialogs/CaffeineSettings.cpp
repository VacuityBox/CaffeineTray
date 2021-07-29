#include "CaffeineSettings.hpp"

#include "Dialogs/AddDialog.hpp"
#include "Dialogs/EditDialog.hpp"
#include "Dialogs/AddWizard.hpp"
#include "Utility.hpp"

namespace Caffeine {

constexpr auto COLUMN_VALUE_TEXT = L"Process Name/Path | Window Title";
constexpr auto COLUMN_TYPE_TEXT  = L"Type";

constexpr auto ITEM_TYPE_NAME_STRING    = L"Process Name";
constexpr auto ITEM_TYPE_PATH_STRING    = L"Process Path";
constexpr auto ITEM_TYPE_WINDOW_STRING  = L"Window Title";

auto CaffeineSettings::OnInit (HWND dlgHandle) -> bool
{
    mListViewItems = GetDlgItem(dlgHandle, IDC_LISTVIEW_PROCESSES_AND_WINDOWS);
    mButtonAdd    = GetDlgItem(dlgHandle, IDC_BUTTON_AUTO_ADD);
    mButtonWizard = GetDlgItem(dlgHandle, IDC_BUTTON_AUTO_ADD_WIZARD);
    mButtonEdit   = GetDlgItem(dlgHandle, IDC_BUTTON_AUTO_EDIT);
    mButtonRemove = GetDlgItem(dlgHandle, IDC_BUTTON_AUTO_REMOVE);

    // ListView need LVS_OWNERDATA and LVS_SHAREIMAGELISTS.
    // Create columns.
    auto listView = GetDlgItem(dlgHandle, IDC_LISTVIEW_PROCESSES_AND_WINDOWS);

    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT);

    auto lvc = LVCOLUMN{ 0 };
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.cx       = 660;
    lvc.pszText  = const_cast<LPWSTR>(COLUMN_VALUE_TEXT);
    ListView_InsertColumn(listView, 0, &lvc);
        
    lvc.iSubItem = 1;
    lvc.cx       = 100;
    lvc.pszText  = const_cast<LPWSTR>(COLUMN_TYPE_TEXT);
    ListView_InsertColumn(listView, 1, &lvc);
    
    // Set image list.
    ListView_SetImageList(listView, mIconCache->GetImageList(), LVSIL_SMALL);

    // Fill the list.
    Refresh();

    // Set checkboxes state.
    Button_SetCheck(
        GetDlgItem(dlgHandle, IDC_CHECKBOX_STANDARD_KEEP_DISPLAY_ON),
        mCurrentSettings->Standard.KeepDisplayOn ? BST_CHECKED : BST_UNCHECKED
    );

    Button_SetCheck(
        GetDlgItem(dlgHandle, IDC_CHECKBOX_STANDARD_DISABLE_ON_LOCK_SCREEN),
        mCurrentSettings->Standard.DisableOnLockScreen ? BST_CHECKED : BST_UNCHECKED
    );
        
    Button_SetCheck(
        GetDlgItem(dlgHandle, IDC_CHECKBOX_AUTO_KEEP_DISPLAY_ON),
        mCurrentSettings->Auto.KeepDisplayOn ? BST_CHECKED : BST_UNCHECKED
    );

    Button_SetCheck(
        GetDlgItem(dlgHandle, IDC_CHECKBOX_AUTO_DISABLE_ON_LOCK_SCREEN),
        mCurrentSettings->Auto.DisableOnLockScreen ? BST_CHECKED : BST_UNCHECKED
    );

    // Disable edit/remove button because noting is selected.
    EnableWindow(mButtonEdit, FALSE);
    EnableWindow(mButtonRemove, FALSE);

    if (mItems->Count() == 0)
    {
        DisplayWarning();
    }

    return true;
}

auto CaffeineSettings::OnOk () -> bool
{
    // Store listview values.
    mResult = mItems->ToSettings();

    // Store checkbox values.
    mResult.Standard.KeepDisplayOn = Button_GetCheck(
        GetDlgItem(mDlgHandle, IDC_CHECKBOX_STANDARD_KEEP_DISPLAY_ON)
    ) == BST_CHECKED;

    mResult.Standard.DisableOnLockScreen = Button_GetCheck(
        GetDlgItem(mDlgHandle, IDC_CHECKBOX_STANDARD_DISABLE_ON_LOCK_SCREEN)
    ) == BST_CHECKED;

    mResult.Auto.KeepDisplayOn = Button_GetCheck(
        GetDlgItem(mDlgHandle, IDC_CHECKBOX_AUTO_KEEP_DISPLAY_ON)
    ) == BST_CHECKED;

    mResult.Auto.DisableOnLockScreen = Button_GetCheck(
        GetDlgItem(mDlgHandle, IDC_CHECKBOX_AUTO_DISABLE_ON_LOCK_SCREEN)
    ) == BST_CHECKED;

    return true;
}

auto CaffeineSettings::OnCancel () -> bool
{
    return true;
}

auto CaffeineSettings::OnCommand (WPARAM wParam, LPARAM lParam) -> bool
{
    bool deactivateButtons = true;

    switch (LOWORD(wParam))
    {
    case IDC_BUTTON_AUTO_ADD:
    {
        auto addDlg = AddDialog(mItems);
        if (addDlg.Show(mDlgHandle, true))
        {
            // Get returned values.
            const auto& result = addDlg.Result();

            auto icon = FindIcon(result.value, result.type);

            InsertItem(Item(result.value, result.type, icon));

            deactivateButtons = !SetSelectedItem(mItems->Count() - 1);
        }
            
        break;
    }

    case IDC_BUTTON_AUTO_ADD_WIZARD:
    {
        auto addWizard = AddWizardDialog(mItems, mIconCache, mRunningProcesses);
        if (addWizard.Show(mDlgHandle, true))
        {
            // Get returned values.
            const auto& result = addWizard.Result();
            InsertItem(Item(result.value, result.type, result.icon));

            deactivateButtons = !SetSelectedItem(mItems->Count() - 1);
        }
        break;
    }

    case IDC_BUTTON_AUTO_EDIT:
    {
        auto selectedIndex = GetSelectedIndex(false);
        if (selectedIndex < 0 || selectedIndex >= mItems->Count())
        {
            break;
        }

        // Show dialog with already filled value and type.
        auto editDlg = EditDialog(mItems, selectedIndex);
        if (editDlg.Show(mDlgHandle, true))
        {
            // Get returned values.
            const auto& result = editDlg.Result();

            auto icon = FindIcon(result.value, result.type);
            ModifyItem(selectedIndex, Item(result.value, result.type, icon));
        }

        deactivateButtons = !SetSelectedItem(selectedIndex);

        break;
    }

    case IDC_BUTTON_AUTO_REMOVE:
    {
        auto selectedIndex = GetSelectedIndex(true);
        if (selectedIndex < 0 || selectedIndex >= mItems->Count())
        {
            break;
        }

        RemoveItem(selectedIndex);
            
        break;
    }

    }

    if (deactivateButtons)
    {
        EnableWindow(mButtonEdit, FALSE);
        EnableWindow(mButtonRemove, FALSE);
    }

    if (mItems->Count() == 0)
    {
        DisplayWarning();
    }
    else
    {
        HideWarning();
    }

    return true;
}

auto CaffeineSettings::OnNotify (WPARAM wParam, LPARAM lParam) -> bool
{
    switch (LOWORD(wParam))
    {

    case IDC_LISTVIEW_PROCESSES_AND_WINDOWS:
    {
        switch (reinterpret_cast<LPNMHDR>(lParam)->code)
        {

        case NM_CLICK:
        {
            // Enable Edit/Remove buttons when something selected.
            auto lpnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

            EnableWindow(mButtonEdit  , lpnmia->iItem != -1);
            EnableWindow(mButtonRemove, lpnmia->iItem != -1);
            break;
        }        
        
        case NM_SETFOCUS:
        {
            // Enable Edit/Remove buttons when something selected.
            auto lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
            auto index = ListView_GetSelectionMark(mListViewItems);
            if (index == -1)
            {
                if (mItems->Count() > 0)
                    index = 0;    
            }
            SetSelectedItem(index);
            break;
        }

        case NM_KILLFOCUS:
        {
            break;
        }

        case LVN_ITEMCHANGED:
        {
            // Enable Edit/Remove buttons when something selected.
            auto lpnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);

            EnableWindow(mButtonEdit  , lpnmlv->iItem != -1);
            EnableWindow(mButtonRemove, lpnmlv->iItem != -1);
            break;
        }

        case LVN_GETDISPINFO:
        {
            auto nmlvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
            auto index  = nmlvdi->item.iItem;
            if (index == -1)
            {
                break;
            }

            auto& items = mItems->GetItems();
            if (index >= items.size())
            {
                break;
            }

            // Tell listview how to display items.
            if (nmlvdi->item.mask & LVIF_TEXT)
            {
                switch (nmlvdi->item.iSubItem)
                {
                // Column 0.
                case 0:
                {
                    auto& item = items[index];

                    nmlvdi->item.pszText = item.value.data();

                    if (item.icon != INVALID_ICON_ID)
                    {
                        nmlvdi->item.iImage = item.icon;
                    }
                    break;
                }

                // Column Type.
                case 1:
                {
                    auto& item = items[index];

                    switch (item.type)
                    {
                    case ItemType::Name:
                        nmlvdi->item.pszText = const_cast<LPWSTR>(ITEM_TYPE_NAME_STRING);
                        break;
                    case ItemType::Path:
                        nmlvdi->item.pszText = const_cast<LPWSTR>(ITEM_TYPE_PATH_STRING);
                        break;
                    case ItemType::Window:
                        nmlvdi->item.pszText = const_cast<LPWSTR>(ITEM_TYPE_WINDOW_STRING);
                        break;
                    }
                    break;
                }
                }
            }

            break;
        } // case LVN_GETDISPINFO:

        } // switch (reinterpret_cast<LPNMHDR>(lParam)->code)
    } // case IDC_LISTVIEW_PROCESSES_AND_WINDOWS:

    } // switch (LOWORD(wParam))

    return true;
}

auto CaffeineSettings::OnClose () -> void
{
}

auto CaffeineSettings::InsertItem (Item item) -> bool
{
    if (mItems->Push(item))
    {
        // Update listview.
        ListView_SetItemCountEx(mListViewItems, mItems->Count(), LVSICF_NOSCROLL);
        
        return true;
    }

    return false;
}

auto CaffeineSettings::ModifyItem (int index, Item item) -> bool
{
    // Modify existing value. Check if already exist is done in AddEditDialog.
    if (index >= mItems->Count())
    {
        return false;
    }

    auto& items = mItems->GetItems();
    items[index] = item;

    // Trigger refresh of the item.
    ListView_RedrawItems(mListViewItems, index, index);

    return true;
}

auto CaffeineSettings::RemoveItem (int index) -> bool
{
    if (index >= mItems->Count())
    {
        return false;
    }

    if (mItems->Remove(index))
    {
        // Update listview.
        ListView_SetItemCountEx(mListViewItems, mItems->Count(), LVSICF_NOSCROLL);
        return true;
    }

    return false;
}

auto CaffeineSettings::Refresh () -> void
{
    // Update listview.
    ListView_SetItemCountEx(mListViewItems, mItems->Count(), LVSICF_NOSCROLL);
}

auto CaffeineSettings::FindIcon (const std::wstring& name, ItemType type) -> int
{
    auto findIcon = [&](const std::wstring& name)
    {
        for (const auto& p : mRunningProcesses->Get())
        {
            switch (type)
            {
            case ItemType::Name:
                if (name == p.second.name)
                {
                    return p.second.icon;
                }
                break;
            case ItemType::Path:    
                if (name == p.second.path)
                {
                    return p.second.icon;
                }
                break;
            case ItemType::Window:
                if (name == p.second.window)
                {
                    return p.second.icon;
                }
                break;
            }
        }

        return INVALID_ICON_ID;
    };

    mRunningProcesses->Refresh();

    switch (type)
    {
    case ItemType::Name:
    case ItemType::Window:
        return findIcon(name);     

    case ItemType::Path:
        return mIconCache->Insert(name);          
    }

    return INVALID_ICON_ID;
}

auto CaffeineSettings::GetSelectedIndex (bool deselectAfter) -> int
{
    auto selectedIndex = ListView_GetSelectionMark(mListViewItems);

    if (selectedIndex != -1 && deselectAfter)
    {
        ListView_SetSelectionMark(mListViewItems, -1);
    }

    return selectedIndex;
}

auto CaffeineSettings::SetSelectedItem (int index) -> bool
{
    if (index < 0 || index >= mItems->Count())
    {
        return false;
    }
    
    // Set focus and selected item to index.
    ListView_SetSelectionMark(mListViewItems, index);
    ListView_SetItemState(mListViewItems, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(mListViewItems, index, false);
    SetFocus(mListViewItems);

    return true;
}

auto CaffeineSettings::DisplayWarning () -> void
{
    if (!mWarningShowed)
    {
        auto warningIcon = HICON{0};
        const auto warningText = L"You need to add something to the list to make Auto mode work!";

        LoadIconWithScaleDown(NULL, IDI_WARNING, 16, 16, &warningIcon);

        Static_SetIcon(GetDlgItem(mDlgHandle, IDC_ICON_WARNING), warningIcon);
        SetDlgItemText(mDlgHandle, IDC_TEXT_WARNING, warningText);

        mWarningShowed = true;
    }
}

auto CaffeineSettings::HideWarning () -> void
{
    if (mWarningShowed)
    {
        Static_SetIcon(GetDlgItem(mDlgHandle, IDC_ICON_WARNING), NULL);
        SetDlgItemText(mDlgHandle, IDC_TEXT_WARNING, L"");
    
        mWarningShowed = false;
    }
}

} // namespace Caffeine
