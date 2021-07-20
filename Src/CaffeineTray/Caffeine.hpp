#pragma once

namespace Caffeine {

enum class CaffeineMode : unsigned char
{
    Disabled,
    Enabled,
    Auto
};

class Caffeine final
{
    bool mIsActive;
    bool mKeepDisplayOn;

    Caffeine            (const Caffeine&) = delete;
    Caffeine& operator= (const Caffeine&) = delete;

public:
    Caffeine ()
        : mIsActive      (false)
        , mKeepDisplayOn (false)
    {
    }
    
    ~Caffeine ()
    {
    }

    auto Enable  (bool keepDisplayOn) -> bool;
    auto Disable ()                   -> bool;

    auto IsActive () -> bool { return mIsActive; }
};

} // namespace Caffeine
