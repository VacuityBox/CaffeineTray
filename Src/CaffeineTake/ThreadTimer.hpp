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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

namespace CaffeineTake {

class ThreadTimer;

class StopToken final
{
    friend class ThreadTimer;

    std::atomic<bool> mStopAtomic;

    inline auto Stop () -> void
    {
        mStopAtomic.store(true);
    }

    inline auto Reset () -> void
    {
        mStopAtomic.store(false);
    }

public:
    StopToken ()
        : mStopAtomic (false)
    {
    }

    inline auto Test () const -> bool
    {
        return mStopAtomic.load();
    }

    inline operator bool () const
    {
        return Test();
    }
};

class PauseToken final
{
    friend class ThreadTimer;

    std::atomic<bool> mPauseAtomic;

    inline auto Pause () -> void
    {
        mPauseAtomic.store(true);
    }

    inline auto Notify () -> void
    {
        mPauseAtomic.notify_one();
    }

    inline auto Reset () -> void
    {
        mPauseAtomic.store(false);
    }

public:
    PauseToken ()
        : mPauseAtomic (false)
    {
    }

    inline auto Test () const -> bool
    {
        return mPauseAtomic.load();
    }

    inline operator bool () const
    {
        return Test();
    }

    inline auto Wait () const -> void
    {
        mPauseAtomic.wait(true);
    }
};

class ThreadTimer
{
public:
    using CallbackFn = std::function<bool (const StopToken&, const PauseToken&)>;
    using Interval   = std::chrono::milliseconds;

private:
    CallbackFn                mTimerCallback          = nullptr;         // return false to stop
    Interval                  mInterval               = Interval(0);
    std::thread               mWorkerThread;
    std::mutex                mWorkerMutex;
    std::condition_variable   mWorkerConditionVar;
    std::atomic<bool>         mWorkerRunning          = false;
    std::atomic<bool>         mIsDone                 = true;
    std::atomic<bool>         mIsPaused               = false;
    std::atomic<bool>         mIsWaiting              = false;
    std::atomic<bool>         mInCallback             = false;
    const bool                mRunCallbackImmediately = false;           // run callback immediately after worker start
    StopToken                 mStopToken              = StopToken();
    PauseToken                mPauseToken             = PauseToken();

    auto Worker () -> void
    {
        {
            auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
            mWorkerRunning = true;
        }

        auto result = true;
        if (mRunCallbackImmediately)
        {
            {
                auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
                mInCallback = true;
            }
            result = mTimerCallback(mStopToken, mPauseToken);
            {
                auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
                mInCallback = false;
            }
        }

        if (result)
        {
            while (true)
            {
                // Wait for specific interval.
                {
                    auto waitLock  = std::unique_lock<std::mutex>(mWorkerMutex);
                    mIsWaiting = true;
                    mWorkerConditionVar.wait_for(
                        waitLock,
                        mInterval,
                        [&]
                        {
                            return mIsPaused || mIsDone; // return false to continue wait
                        }
                    );
                    mIsWaiting = false;
                }

                // Check if we finished.
                mWorkerMutex.lock();
                if (mIsDone)
                {
                    mWorkerMutex.unlock();
                    break;
                }

                if (mIsPaused)
                {
                    mWorkerMutex.unlock();
                    {
                        auto waitLock  = std::unique_lock<std::mutex>(mWorkerMutex);
                        mWorkerConditionVar.wait(
                            waitLock,
                            [&]
                            {
                                return !mIsPaused || mIsDone; // return false to continue wait
                            }
                        );
                    }
                    mWorkerMutex.lock();
                }
                else
                {
                    mInCallback = true;
                    mWorkerMutex.unlock();
                    if (!mTimerCallback(mStopToken, mPauseToken))
                    {
                        break;
                    }
                    mWorkerMutex.lock();
                    mInCallback = false;
                }
                
                // Check if we finished.
                if (mIsDone)
                {
                    mWorkerMutex.unlock();
                    break;
                }

                mWorkerMutex.unlock();
            }
        }

        {
            auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
            mWorkerRunning = false;
        }
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
        : mTimerCallback          (callback)
        , mInterval               (interval)
        , mIsDone                 (true)
        , mIsPaused               (false)
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
        else if (mTimerCallback == nullptr)
        {
            result = false;
        }
        else
        {
            auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);

            if (mIsDone)
            {
                // If worker quit from callback, we need to join.
                if (mWorkerThread.joinable())
                {
                    mWorkerThread.join();
                }

                mStopToken.Reset();
                mPauseToken.Reset();

                mIsDone       = false;
                mIsPaused     = false;
                mWorkerThread = std::thread(&ThreadTimer::Worker, this);
            }

            if (mIsPaused)
            {
                mIsPaused = false;
                mPauseToken.Reset();

                if (mInCallback)
                {
                    mPauseToken.Notify();
                }
                else
                {
                    mWorkerConditionVar.notify_one();
                }
            }
        }

        return result;
    }

    auto Stop () -> void
    {
        {
            auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
        
            mIsDone = true;
            mStopToken.Stop();

            if (mIsPaused)
            {
                mIsPaused = false;
                mPauseToken.Reset();
            
                if (mInCallback)
                {
                    mPauseToken.Notify();
                }
                else
                {
                    mWorkerConditionVar.notify_one();
                }
            }

            if (mIsWaiting)
            {
                mWorkerConditionVar.notify_one();
            }
        }

        if (mWorkerThread.joinable())
        {
            mWorkerThread.join();
        }
    }

    auto Pause () -> void
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);

        if (!mIsDone && !mIsPaused)
        {
            mIsPaused = true;
            mPauseToken.Pause();
        }
    }

    auto SetCallback (CallbackFn callback) -> bool
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);

        if (mIsDone)
        {
            mTimerCallback = callback;
        }

        return mIsDone;
    }

    auto SetInterval (Interval interval) -> bool
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);

        if (mIsDone)
        {
            mInterval = interval;
        }

        return mIsDone;
    }

    auto GetInterval () const -> Interval
    {
        return mInterval;
    }

    auto IsRunning () -> bool
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
        return !mIsDone;
    }

    auto IsPaused () -> bool
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
        return !mIsDone && mIsPaused;
    }

    auto IsStopped () -> bool
    {
        auto lockGuard = std::lock_guard<std::mutex>(mWorkerMutex);
        return mIsDone;
    }
};

} // namespace CaffeineTake
