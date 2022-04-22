#pragma once

#include <string_view>

namespace CaffeineTake {

enum class CaffeineMode : unsigned char
{
    Disabled,
    Enabled,
    Auto
};

constexpr auto CaffeineModeToString (CaffeineMode mode) -> std::wstring_view
{
    switch (mode)
    {
    case CaffeineMode::Disabled: return L"Disabled";
    case CaffeineMode::Enabled:  return L"Enabled";
    case CaffeineMode::Auto:     return L"Auto";
    }

    return L"Invalid CaffeineMode";
}

} // namespace CaffeineTake
