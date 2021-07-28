#pragma once

#include "Helpers/AddEditDialog.hpp"
#include "Helpers/DialogResult.hpp"

namespace Caffeine {

class AddDialog : public AddEditDialog<AddDialogResult>
{
    virtual auto OnInit (HWND dlgHandle) -> bool override
    {
        EnableWindow(GetDlgItem(dlgHandle, IDOK), FALSE);
        
        // To make focus work we need to return false.
        SetFocus(GetDlgItem(dlgHandle, IDC_EDITBOX_ADD_EDIT));
        return false;
    }

    virtual auto OnOk () -> bool override
    {
        auto vt = GetValueAndType();
        if (!vt.has_value())
        {
            return false;
        }

        auto [value, type] = vt.value();

        // Check if item already exists.
        if (mItems->Exists(value, type))
        {
            ShowErrorMessageBox(type);

            return false;
        }

        this->mResult.value = value;
        this->mResult.type  = type;

        return true;
    }

public:
    AddDialog (std::shared_ptr<ItemList> items)
        : AddEditDialog (items, L"Add")
    {
    }
};

} // namespace Caffeine
