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

#include "PCH.hpp"
#include "Config.hpp"
#include "CaffeineSounds.hpp"
#include "Resource.hpp"

#if defined(FEATURE_CAFFEINETAKE_NOTIFICATION_SOUND)
#   include "Logger.hpp"
#   include <fstream>
#   include <mmsystem.h>
#endif

namespace CaffeineTake {

auto CaffeineSounds::LoadOriginalSounds () -> bool
{
    return true;
}

auto CaffeineSounds::LoadSquareSounds () -> bool
{
    return true;
}

auto CaffeineSounds::LoadCustomSounds () -> bool
{
#if defined(FEATURE_CAFFEINETAKE_CUSTOM_SOUNDS)
    auto loadWav = [&](fs::path path)
    {
        auto f = std::ifstream(path, std::ios::binary);
        if (!f)
        {
            LOG_ERROR("Failed to load file '{}'", path.string());
            return std::vector<BYTE>();
        }

        auto err = std::error_code{};
        auto size = fs::file_size(path, err);
        if (err)
        {
            LOG_ERROR("Failed to get file size '{}'", path.string());
            return std::vector<BYTE>();
        }

        auto limit = 1024*1024;
        if (size > limit)
        {
            LOG_ERROR("File too big '{}'", path.string());
            return std::vector<BYTE>();
        }

        auto buffer = std::vector<BYTE>(size);
        f.read(reinterpret_cast<char*>(buffer.data()), size);
    };

    mActiveWav = loadWav(mCustomSoundsPath / "Active.wav");
    mInactiveWav = loadWav(mCustomSoundsPath / "Inactive.wav");
#endif
    return true;
}

CaffeineSounds::CaffeineSounds (HINSTANCE hInstance)
    : mInstanceHandle (hInstance)
{
}

CaffeineSounds::CaffeineSounds (HINSTANCE hInstance, fs::path customSoundsPath)
    : mInstanceHandle   (hInstance)
    , mCustomSoundsPath (customSoundsPath)
{
}

CaffeineSounds::~CaffeineSounds()
{
    mInstanceHandle = NULL;
}

auto CaffeineSounds::Load (SoundPack pack) -> bool
{
    mPack = pack;

    switch (mPack)
    {
    case SoundPack::Original: return LoadOriginalSounds();
    case SoundPack::Square:   return LoadSquareSounds();
    case SoundPack::Custom:   return LoadCustomSounds();
    case SoundPack::System:   return true;
    }

    return false;
}

auto CaffeineSounds::PlayActivateSound () -> void
{
#if defined(FEATURE_CAFFEINETAKE_NOTIFICATION_SOUND)
    switch (mPack)
    {
        case SoundPack::Original:
            PlaySoundW(MAKEINTRESOURCEW(IDW_ORIGINAL_ACTIVE), mInstanceHandle, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
            break;
        case SoundPack::Square:
            break;
        case SoundPack::Custom:
            PlaySoundW(reinterpret_cast<LPCWSTR>(mActiveWav.data()), mInstanceHandle, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
            break;
        case SoundPack::System:
            PlaySoundW(reinterpret_cast<LPCWSTR>(SND_ALIAS_SYSTEMDEFAULT), mInstanceHandle, SND_ALIAS_ID | SND_ASYNC | SND_NODEFAULT);
            break;
    }
#endif
}

auto CaffeineSounds::PlayDeactivateSound () -> void
{
#if defined(FEATURE_CAFFEINETAKE_NOTIFICATION_SOUND)
    switch (mPack)
    {
        case SoundPack::Original:
            PlaySoundW(MAKEINTRESOURCEW(IDW_ORIGINAL_INACTIVE), mInstanceHandle, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
            break;
        case SoundPack::Square:
            break;
        case SoundPack::Custom:
            PlaySoundW(reinterpret_cast<LPCWSTR>(mInactiveWav.data()), mInstanceHandle, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
            break;
        case SoundPack::System:
            PlaySoundW(reinterpret_cast<LPCWSTR>(SND_ALIAS_SYSTEMDEFAULT), mInstanceHandle, SND_ALIAS_ID | SND_ASYNC | SND_NODEFAULT);
            break;
    }
#endif
}

} // namespace CaffeineTake
