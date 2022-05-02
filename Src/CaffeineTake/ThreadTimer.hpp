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

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

namespace CaffeineTake {

class ThreadTimer
{
public:
    using CallbackFn = std::function<bool ()>;
    using Interval   = std::chrono::milliseconds;

private:
    CallbackFn                mTimeoutCallback        = nullptr;         // return false to stop
    std::thread               mWorkerThread;
    bool                      mWorkerRunning          = false;           // ro from caller, rw from worker
    bool                      mIsDone                 = true;            // rw from caller, ro from worker
    std::condition_variable   mPauseConditionVar;
    std::mutex                mPauseMutex;
    bool                      mIsPaused               = false;           // rw from caller, ro from worker
    Interval                  mInterval               = Interval(0);
    bool                      mRunCallbackImmediately = false;           // run callback immediately after worker start

    auto Worker () -> void
    {
        mWorkerRunning = true;

        if (mTimeoutCallback)
        {
            auto pauseLock = std::unique_lock<std::mutex>(mPauseMutex);

            auto result = true;
            if (mRunCallbackImmediately)
            {
                result = mTimeoutCallback();
            }

            if (result)
            {
                while (!mIsDone)
                {
                    std::this_thread::sleep_for(mInterval);

                    if (!mIsDone)
                    {
                        if (!mIsPaused)
                        {
                            if (!mTimeoutCallback())
                            {
                                break;
                            }
                        }
                        else
                        {
                            mPauseConditionVar.wait(
                                pauseLock,
                                [&]
                                {
                                    return !mIsPaused || mIsDone; // return false to continue wait
                                }
                            );
                        }
                    }
                }
            }
        }

        mWorkerRunning = false;
    }

    ThreadTimer            (const ThreadTimer& rhs) = delete;
    ThreadTimer& operator= (const ThreadTimer& rhs) = delete;

public:
    ThreadTimer (
        CallbackFn callback,
        Interval   interval            = Interval(1000),
        bool       autoStart           = false,
        bool       callbackImmediately = false
    )
        : mTimeoutCallback        (callback)
        , mInterval               (interval)
        , mIsDone                 (true)
        , mIsPaused               (false)
        , mWorkerRunning          (false)
        , mRunCallbackImmediately (callbackImmediately)
    {
        if (autoStart)
        {
            Start();
        }
    }

    ~ThreadTimer ()
    {
        Stop();
    }

    auto Start () -> bool
    {
        auto result = true;

        if (mInterval <= Interval(0))
        {
            result = false;
        }
        else if (mTimeoutCallback == nullptr)
        {
            result = false;
        }
        else
        {
            if (!mWorkerRunning)
            {
                // If worker quit from callback, we need to join.
                if (mWorkerThread.joinable())
                {
                    mWorkerThread.join();
                }

                mIsDone       = false;
                mIsPaused     = false;
                mWorkerThread = std::thread(&ThreadTimer::Worker, this);
            }

            if (mIsPaused)
            {
                mIsPaused = false;
                mPauseConditionVar.notify_one();
            }
        }

        return result;
    }

    auto Stop () -> void
    {
        mIsDone = true;
        if (mIsPaused)
        {
            mIsPaused = false;
            mPauseConditionVar.notify_one();
        }

        if (mWorkerRunning || mWorkerThread.joinable())
        {
            mWorkerThread.join();
        }
    }

    auto Pause () -> void
    {
        if (!mIsPaused && mWorkerRunning)
        {
            mIsPaused = true;
        }
    }

    auto SetCallback (CallbackFn callback) -> bool
    {
        if (!mWorkerRunning)
        {
            mTimeoutCallback = callback;            
        }

        return !mWorkerRunning;
    }

    auto SetInterval (Interval interval) -> bool
    {
        if (!mWorkerRunning)
        {
            mInterval = interval;            
        }

        return !mWorkerRunning;
    }

    auto GetInterval () const -> Interval
    {
        return mInterval;
    }

    auto IsRunning () const -> bool { return mWorkerRunning; }
    auto IsPaused  () const -> bool { return mWorkerRunning && mIsPaused; }
    auto IsStopped () const -> bool { return !mWorkerRunning; }
};

} // namespace CaffeineTake
