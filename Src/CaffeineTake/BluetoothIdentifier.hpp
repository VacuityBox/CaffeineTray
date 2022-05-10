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

} // namespace CaffeineTake
