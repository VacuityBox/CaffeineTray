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
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

// Base class for dialogs windows.
template <typename T>
class Dialog
{
protected:
    HWND mDlgHandle;
    T    mResult;

private:
    bool         mResultFlag;
    int          mDlgTemplateId;
    std::wstring mTitle;

    virtual auto OnInit    (HWND dlgHandle)               -> bool = 0;
    virtual auto OnOk      ()                             -> bool = 0;
    virtual auto OnCancel  ()                             -> bool = 0;
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool = 0;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool = 0;
    virtual auto OnClose   ()                             -> void = 0;

    virtual auto UserDlgProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        return FALSE;
    }

    auto Dispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

    static auto CALLBACK DlgProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR;

    auto Reset () -> void
    {
        mDlgHandle  = NULL;
        mResultFlag = false;
        mResult     = T();
    }

protected:
    // Use when you want to manualy destroy dialog and set result.
    auto ResultOk () -> void
    {
        mResultFlag = true;
        DestroyWindow(mDlgHandle);
    }

    auto ResultCancel () -> void
    {
        mResultFlag = false;
        DestroyWindow(mDlgHandle);
    }

private:
    Dialog            (const Dialog&) = delete;
    Dialog& operator= (const Dialog&) = delete;

protected:
    explicit Dialog (int dlgTemplateId,  std::wstring_view title = L"")
        : mDlgHandle     (NULL)
        , mResult        ()
        , mResultFlag    (false)
        , mDlgTemplateId (dlgTemplateId)
        , mTitle         (title)
    {
    }

public:
    virtual ~Dialog() {}

    auto IsCreated    () -> bool { return mDlgHandle != NULL; }
    auto GetDlgHandle () -> HWND { return mDlgHandle; }

    auto Result () -> const T&
    {
        return mResult;
    }

    auto Show (HWND parent, bool modal = false) -> bool;
};

} // namespace CaffeineTake

// Dialog definitions.
namespace CaffeineTake {

template <typename T>
auto Dialog<T>::Dispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{
    switch (message)
    {
    case WM_INITDIALOG:
        mDlgHandle = hWnd;
        return OnInit(hWnd);

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (OnOk())
            {
                mResultFlag = true;
                DestroyWindow(hWnd);
            }
            return TRUE;            

        case IDCANCEL:
            if (OnCancel())
            {
                mResultFlag = false;
                DestroyWindow(hWnd);
            }
            return TRUE;
        }
            
        return OnCommand(wParam, lParam);
    }

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_CLOSE:
        OnClose();

        mResultFlag = false;
        DestroyWindow(hWnd);
        return TRUE;

    case WM_DESTROY:
        mDlgHandle = NULL;
        return TRUE;
    }

    return UserDlgProc(hWnd, message, wParam, lParam);
}

template <typename T>
auto Dialog<T>::DlgProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR
{
    if (message == WM_INITDIALOG)
    {
        auto dlgPtr = reinterpret_cast<Dialog*>(lParam);
        if (dlgPtr)
        {
            SetWindowLongPtr(hWnd, DWLP_USER, static_cast<LONG_PTR>(lParam));
            return dlgPtr->Dispatch(hWnd, message, wParam, lParam);
        }
    }
    else
    {
        auto dlgPtr = reinterpret_cast<Dialog*>(GetWindowLongPtrW(hWnd, DWLP_USER));
        if (dlgPtr)
        {
            return dlgPtr->Dispatch(hWnd, message, wParam, lParam);
        }
    }

    return FALSE;
}

template <typename T>
auto Dialog<T>::Show (HWND parent, bool modal) -> bool
{
    Reset();
        
    auto hInstance = GetModuleHandle(NULL);

    mDlgHandle = CreateDialogParam(
        hInstance,
        MAKEINTRESOURCE(mDlgTemplateId),
        parent,
        DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    if (!mDlgHandle)
    {
        return false;
    }

    if (modal)
    {
        EnableWindow(parent, FALSE);
    }

    if (!mTitle.empty())
    {
        SetWindowText(mDlgHandle, mTitle.data());
    }

    SetWindowLongPtrW(mDlgHandle, DWLP_USER, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(mDlgHandle, SW_SHOW);

    // Main message loop.
    auto msg = MSG{0};
    while (mDlgHandle)
    {
        // Re-post WM_QUIT to queue.
        auto ret = GetMessage(&msg, NULL, 0, 0);
        if (ret == 0)
        {
            PostQuitMessage(0);
            break;
        }

        if (!IsWindow(mDlgHandle) || !IsDialogMessage(mDlgHandle, &msg)) 
        {
            TranslateMessage(&msg); 
            DispatchMessage(&msg);
        }
    }

    if (modal)
    {
        EnableWindow(parent, TRUE);
    }

    SetForegroundWindow(parent);

    return mResultFlag;
}

} // namespace CaffeineTake
