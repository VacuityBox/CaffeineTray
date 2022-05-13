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

StandardMode::StandardMode (CaffeineAppSO app)
    : Mode (app)
{
}

auto StandardMode::Start () -> bool
{
    mAppSO.EnableCaffeine();

    LOG_TRACE("Started Standard mode");
    return true;
}

auto StandardMode::Stop () -> bool
{
    mAppSO.DisableCaffeine();

    LOG_TRACE("Stopped Standard mode");
    return true;
}

auto StandardMode::GetIcon (CaffeineState state) const -> const HICON
{
    auto icons = mAppSO.GetIcons();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return icons->CaffeineStandardInactive;
    case CaffeineTake::CaffeineState::Active:   return icons->CaffeineStandardActive;
    }

    return NULL;
}

auto StandardMode::GetTip (CaffeineState state) const -> const std::wstring&
{
    auto lang = mAppSO.GetLang();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return lang->Tip_StandardInactive;
    case CaffeineTake::CaffeineState::Active:   return lang->Tip_StandardActive;
    }

    return L"";
}

auto StandardMode::IsModeAvailable () const -> bool
{
    return true;
}

auto StandardMode::GetName () const -> std::wstring_view
{
    return L"Standard";
}

} // namespace CaffeineTake
