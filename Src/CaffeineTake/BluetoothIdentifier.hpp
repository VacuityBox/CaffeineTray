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

#if defined(FEATURE_CAFFEINETAKE_SETTINGS)
#   include "Utility.hpp"
#   include <nlohmann/json.hpp>
#endif

#include <format>
#include <string>

namespace CaffeineTake {

struct BluetoothIdentifier
{
    union
    {
        unsigned long long ull;
        unsigned char      bytes[6];
    };

    BluetoothIdentifier ()
        : ull (0)
    {
    }

    auto operator= (unsigned long long ull) -> BluetoothIdentifier&
    {
        this->ull = ull;
    }

    auto operator== (const BluetoothIdentifier& other) const
    {
        return this->ull == other.ull;
    }

    auto operator!= (const BluetoothIdentifier& other) const
    {
        return this->ull != other.ull;
    }

    auto operator== (unsigned long long ull) const
    {
        return this->ull == ull;
    }

    auto operator!= (unsigned long long ull) const
    {
        return this->ull != ull;
    }

    auto IsValid () const
    {
        return this->ull != 0;
    }

    auto IsInvalid () const
    {
        return this->ull == 0;
    }

    auto ToWString () const -> std::wstring
    {
        return std::format(
            L"{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
            bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]
        );
    }

};

#if defined(FEATURE_CAFFEINETAKE_SETTINGS)
inline auto to_json (nlohmann::json& j, const BluetoothIdentifier& bi) -> void
{
    j = nlohmann::json{
        std::format(
            "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
            bi.bytes[5], bi.bytes[4], bi.bytes[3],
            bi.bytes[2], bi.bytes[1], bi.bytes[0]
        )
    };
}

inline auto from_json (const nlohmann::json& j, BluetoothIdentifier& bi) -> void
{
    const auto s = j.get<std::string>();

    // Example id: 00:00:00:00:00:00
    auto ull = 0ull;
    if (s.length() == 17)
    {
        auto index = 0;
        for (const auto c : s)
        {
            if (index == 2)
            {
                if (c != ':')
                {
                    ull = 0;
                    break;
                }

                index = 0;
            }
            else
            {
                const auto d = HexCharToInt(c);
                if (d == UCHAR_MAX)
                {
                    ull = 0;
                    break;
                }

                ull = (ull << 4) | (d & 0x0F);

                index += 1;
            }
        }
    }

    bi.ull = ull;
}
#endif

} // namespace CaffeineTake
