#pragma once

#include <string_view>

namespace Caffeine {

enum class ItemType : unsigned char
{
    Name    = 0,
    Path    = 1,
    Window  = 2,
    Invalid = 255
};

constexpr auto ItemTypeToString (ItemType type) -> std::wstring_view
{
    switch (type)
    {
    case ItemType::Name:    return L"Name";
    case ItemType::Path:    return L"Path";
    case ItemType::Window:  return L"Window";
    }

    return L"Invalid";
}

constexpr auto StringToItemType (std::wstring_view str) -> ItemType
{
    if (str == L"Name")   return ItemType::Name;
    if (str == L"Path")   return ItemType::Path;
    if (str == L"Window") return ItemType::Window;
        
    return ItemType::Invalid;
}

} // namespace Caffeine
