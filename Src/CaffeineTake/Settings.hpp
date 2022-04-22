#pragma once

#include "CaffeineMode.hpp"
#include "Utility.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

// std::wstring serializer/deserializer.
namespace nlohmann {

template <>
struct adl_serializer<std::wstring>
{
    static void to_json(json& j, const std::wstring& opt)
    {
        auto utf8 = CaffeineTake::UTF16ToUTF8(opt.c_str());
        j = utf8 ? utf8.value() : "";
    }

    static void from_json(const json& j, std::wstring& opt)
    {
        auto utf16 = CaffeineTake::UTF8ToUTF16(j.get<std::string>());
        opt = utf16 ? utf16.value() : L"";
    }
};

} // namespace nlohmann

namespace CaffeineTake {

class Settings final
{
public:
    CaffeineMode Mode;
    bool         UseNewIcons;
    
    struct Standard
    {
        bool KeepDisplayOn;
        bool DisableOnLockScreen;

        Standard ()
            : KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Standard, KeepDisplayOn, DisableOnLockScreen)
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
            , KeepDisplayOn       (true)
            , DisableOnLockScreen (true)
        {
        }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
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
        : Mode        (CaffeineMode::Disabled)
        , UseNewIcons (true)
    {
    }

     NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, Mode, UseNewIcons, Standard, Auto)
};

} // namespace CaffeineTake
