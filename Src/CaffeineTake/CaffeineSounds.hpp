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

#include <filesystem>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class CaffeineSounds
{
public:
    enum class SoundPack : unsigned char
    {
        Original,
        Square,
        Custom,
        System
    };

private:
    HINSTANCE         mInstanceHandle    = NULL;
    fs::path          mCustomSoundsPath  = fs::path();
    std::vector<BYTE> mActiveWav         = std::vector<BYTE>();
    std::vector<BYTE> mInactiveWav       = std::vector<BYTE>();
    SoundPack         mPack              = SoundPack::System;

    auto LoadOriginalSounds () -> bool;
    auto LoadSquareSounds   () -> bool;
    auto LoadCustomSounds   () -> bool;

public:
    CaffeineSounds  (HINSTANCE hInstance);
    CaffeineSounds  (HINSTANCE hInstance, fs::path customSoundsPath);
    ~CaffeineSounds ();

    auto Load (SoundPack pack) -> bool;

    auto PlayActivateSound   () -> void;
    auto PlayDeactivateSound () -> void;
};

}// namespace CaffeineTake
