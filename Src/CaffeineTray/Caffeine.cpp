#include "Caffeine.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Caffeine {

auto Caffeine::Enable (bool keepDisplayOn) -> bool
{
    // Check if not active and request is different.
    if (mIsActive && keepDisplayOn == mKeepDisplayOn)
    {
        return false;
    }
    mKeepDisplayOn = keepDisplayOn;

    auto flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
    if (mKeepDisplayOn)
    {
        flags |= ES_DISPLAY_REQUIRED;
    }

    // Disable previous request before enabling new.
    SetThreadExecutionState(ES_CONTINUOUS);

    // Activate new.
    SetThreadExecutionState(flags);
    mIsActive = true;

    return true;
}

auto Caffeine::Disable () -> bool
{
    if (mIsActive)
    {
        SetThreadExecutionState(ES_CONTINUOUS);
        mIsActive = false;

        return true;
    }

    return false;
}

} // namespace Caffeine
