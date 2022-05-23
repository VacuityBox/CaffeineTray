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
#include "../CaffeineMode.hpp"

#include "Lang.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

namespace CaffeineTake {

DisabledMode::DisabledMode (CaffeineAppSO app)
    : Mode (app)
{
}

auto DisabledMode::Start () -> bool
{
    mAppSO.DisableCaffeine();

    LOG_TRACE("Started Disabled mode");
    return true;
}

auto DisabledMode::Stop () -> bool
{
    LOG_TRACE("Stopped Disabled mode");
    return true;
}

auto DisabledMode::GetIcon (CaffeineState state) const -> const HICON
{
    auto icons = mAppSO.GetIcons();

    return icons->CaffeineStandardInactive;
}

auto DisabledMode::GetTip  (CaffeineState state) const -> const std::wstring&
{
    auto lang = mAppSO.GetLang();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return lang->Tip_DisabledInactive;
    case CaffeineTake::CaffeineState::Active:   return lang->Tip_DisabledActive;
    }

    return L"";
}

auto DisabledMode::IsModeAvailable () const -> bool
{
    return true;
}

auto DisabledMode::GetName () const -> std::wstring_view
{
    return L"Disabled";
}

} // namespace CaffeineTake
