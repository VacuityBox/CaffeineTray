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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CaffeineTake {

class InstanceGuard
{
    HANDLE mGuardMutex;
    bool   mMainInstance;

    InstanceGuard            (const InstanceGuard& rhs) = delete;
    InstanceGuard& operator= (const InstanceGuard& rhs) = delete;

public:
    InstanceGuard ()
        : mGuardMutex   (nullptr)
        , mMainInstance (false)
    {
    }

    ~InstanceGuard ()
    {
        Release();
    }

    auto Protect () -> bool
    {
        mGuardMutex = ::CreateMutexW(nullptr, FALSE, L"CaffeineTake_InstanceGuard");
        if (mGuardMutex != nullptr)
        {
            mMainInstance = GetLastError() != ERROR_ALREADY_EXISTS;

            return true;
        }

        return false;
    }

    auto Release () -> void
    {
        if (mGuardMutex)
        {
            ::CloseHandle(mGuardMutex);
            mGuardMutex = nullptr;
        }
    }

    auto IsMainInstance () -> bool
    {
        return mMainInstance;
    }

    auto IsOtherInstance () -> bool
    {
        return !mMainInstance;
    }
};

} // namespace CaffeineTake
