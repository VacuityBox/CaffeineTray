#pragma once

#include "CaffeineMode.hpp"
#include "Utility.hpp"
#include "json.hpp"

#include <string>
#include <vector>

#define NLOHMANN_DEFINE_TYPE_INTRUSIVE_X(Type, ...) NLOHMANN_DEFINE_TYPE_INTRUSIVE_IMPL(nlohmann::ordered_json, Type, __VA_ARGS__)

// std::wstring serializer/deserializer.
namespace nlohmann {

template <>
struct adl_serializer<std::wstring>
{
    static void to_json(ordered_json& j, const std::wstring& opt)
    {
        auto utf8 = Caffeine::UTF16ToUTF8(opt.c_str());
        j = utf8 ? utf8.value() : "";
    }

    static void from_json(const ordered_json& j, std::wstring& opt)
    {
        auto utf16 = Caffeine::UTF8ToUTF16(j.get<std::string>());
        opt = utf16 ? utf16.value() : L"";
    }
};

} // namespace nlohmann

namespace Caffeine {

class Settings final
{
public:
    CaffeineMode Mode;
    
    struct Standard
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;

        Standard ()
            : KeepDisplayOn       (false)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_X(Standard, KeepDisplayOn, DisableOnLockScreen)
    } Standard;

    struct Auto
    {
        unsigned int ScanInterval;  // in ms
        bool         KeepDisplayOn;
        bool         DisableOnLockScreen;

        std::vector<std::wstring> ProcessNames;
        std::vector<std::wstring> ProcessPaths;
        std::vector<std::wstring> WindowTitles;

        Auto ()
            : ScanInterval        (2000)
            , KeepDisplayOn       (false)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_X(
            Auto,
            ScanInterval,
            KeepDisplayOn,
            DisableOnLockScreen,
            ProcessNames,
            ProcessPaths,
            WindowTitles
        )
    } Auto;

    Settings ()
        : Mode(CaffeineMode::Disabled)
    {
    }

     NLOHMANN_DEFINE_TYPE_INTRUSIVE_X(Settings, Mode, Standard, Auto)
};

} // namespace Caffeine
