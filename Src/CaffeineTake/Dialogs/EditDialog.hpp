#pragma once

#include "Helpers/AddEditDialog.hpp"
#include "Helpers/DialogResult.hpp"

namespace CaffeineTake {

class EditDialog : public AddEditDialog<EditDialogResult>
{
    int mIndexToInitElement;

    virtual auto OnInit (HWND dlgHandle) -> bool override
    {
        if (mIndexToInitElement < 0 || mItems->Count() <= mIndexToInitElement)
        {
            return true;
        }

        const auto& items = mItems->GetItems();
        const auto& value = items[mIndexToInitElement].value;
        const auto& type  = items[mIndexToInitElement].type;

        // Fill the controls with init data.
        auto editControl = GetDlgItem(dlgHandle, IDC_EDITBOX_ADD_EDIT);
        if (type != ItemType::Invalid)
        {
            SetCheckedRadio(dlgHandle, type);
        
            Edit_SetText(editControl, value.c_str());
            Edit_SetSel(editControl, value.length(), value.length());
        
            EnableWindow(GetDlgItem(dlgHandle, IDOK), TRUE);
        }
        
        // To make focus work we need to return false.
        SetFocus(editControl);
        return false;
    }

    virtual auto OnOk () -> bool override
    {
        auto vt = GetValueAndType();
        if (!vt.has_value())
        {
            return true;
        }

        auto [value, type] = vt.value();

        // Check if item already exists.
        const auto& items = mItems->GetItems();
        for (auto index = 0; index < items.size(); index += 1)
        {
            // Skip check of editing element.
            if (index == mIndexToInitElement)
            {
                continue;
            }

            const auto& item = items.at(index);
            if (type == item.type && value == item.value)
            {
                ShowErrorMessageBox(item.type);
                return false;
            }
        }

        // Update the result.
        this->mResult.value = value;
        this->mResult.type  = type;

        return true;
    }

public:
    EditDialog (std::shared_ptr<ItemList> items, int indexToInitElement)
        : AddEditDialog       (items, L"Edit")
        , mIndexToInitElement (indexToInitElement)
    {
    }
};

} // namespace CaffeineTake
