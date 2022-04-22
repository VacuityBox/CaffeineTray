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
        mGuardMutex = ::CreateMutexW(nullptr, FALSE, L"CaffeineTray_InstanceGuard");
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

    auto IsSecondInstance () -> bool
    {
        return !mMainInstance;
    }
};

} // namespace CaffeineTake
