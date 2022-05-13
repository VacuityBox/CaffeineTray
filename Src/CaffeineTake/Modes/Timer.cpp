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

auto TimerMode::TimerProc (const StopToken& stop, const PauseToken& pause) -> bool
{
    mAppSO.DisableCaffeine();

    return false;
}

TimerMode::TimerMode (CaffeineAppSO app)
    : Mode         (app)
    , mTimerThread
        ( std::bind(&TimerMode::TimerProc, this, std::placeholders::_1, std::placeholders::_2)
        , ThreadTimer::Interval(1000)
        , false
        , false
        )
{
}

auto TimerMode::Start () -> bool
{
    const auto settingsPtr = mAppSO.GetSettings();
    if (settingsPtr)
    {
        mTimerThread.SetInterval(std::chrono::milliseconds(settingsPtr->Timer.Interval));
    }

    if (settingsPtr->Timer.Interval == 0)
    {
        LOG_ERROR("Failed to start TimerMode, Interval is 0");
        return false;
    }

    mAppSO.EnableCaffeine();
    mTimerThread.Start();

    LOG_TRACE("Started Timer mode");

    return true;
}

auto TimerMode::Stop () -> bool
{
    mTimerThread.Stop();
    mAppSO.DisableCaffeine();

    LOG_TRACE("Stopped Timer mode");

    return true;
}

auto TimerMode::GetIcon (CaffeineState state) const -> const HICON
{
    auto icons = mAppSO.GetIcons();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return icons->CaffeineTimerInactive;
    case CaffeineTake::CaffeineState::Active:   return icons->CaffeineTimerActive;
    }

    return NULL;
}

auto TimerMode::GetTip (CaffeineState state) const -> const std::wstring&
{
    auto lang = mAppSO.GetLang();

    switch (state)
    {
    case CaffeineTake::CaffeineState::Inactive: return lang->Tip_TimerInactive;
    case CaffeineTake::CaffeineState::Active:   return lang->Tip_TimerActive;
    }

    return L"";
}

auto TimerMode::IsModeAvailable () const -> bool
{
#if defined(FEATURE_CAFFEINETAKE_TIMER_MODE)
    return true;
#else
    return false;
#endif
}

auto TimerMode::GetName () const -> std::wstring_view
{
    return L"Timer";
}

} // namespace CaffeineTake
