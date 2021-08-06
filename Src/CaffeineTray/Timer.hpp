#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

namespace Caffeine {

class Timer
{
    std::function<void ()>    mTickCallback;
    std::thread               mWorkerThread;
    bool                      mWorkerSpawned;
    bool                      mIsDone;
    std::condition_variable   mPauseConditionVar;
    std::mutex                mPauseMutex;
    std::chrono::milliseconds mInterval;
    bool                      mIsPaused;

    auto Worker () -> void
    {
        auto pauseLock = std::unique_lock<std::mutex>(mPauseMutex);
        while (!mIsDone)
        {
            mTickCallback();
            std::this_thread::sleep_for(std::chrono::milliseconds(mInterval));

            if (mIsPaused)
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

    Timer            (const Timer& rhs) = delete;
    Timer& operator= (const Timer& rhs) = delete;

public:
    Timer (
        decltype(mTickCallback) fn,
        decltype(mInterval) interval = std::chrono::milliseconds(1000),
        bool autoStart = false
    )
        : mTickCallback  (fn)
        , mInterval      (interval)
        , mIsDone        (true)
        , mIsPaused      (false)
        , mWorkerSpawned (false)
    {
        if (autoStart)
        {
            Start();
        }
    }

    ~Timer ()
    {
        Stop();
    }

    auto Start () -> void
    {
        if (!mWorkerSpawned)
        {
            mIsDone        = false;
            mIsPaused      = false;
            mWorkerThread  = std::thread(&Timer::Worker, this);
            mWorkerSpawned = true;
        }

        if (mIsPaused)
        {
            mIsPaused = false;
            mPauseConditionVar.notify_one();
        }
    }

    auto Stop () -> void
    {
        mIsDone = true;
        if (mIsPaused)
        {
            mIsPaused = false;
            mPauseConditionVar.notify_one();
        }

        if (mWorkerSpawned)
        {
            mWorkerThread.join();
            mWorkerSpawned = false;
        }
    }

    auto Pause () -> void
    {
        if (!mIsPaused && mWorkerSpawned)
        {
            mIsPaused = true;
        }
    }

    auto Interval (std::chrono::milliseconds interval) -> void
    {
        if (interval < std::chrono::milliseconds(1000))
        {
            mInterval = std::chrono::milliseconds(1000);
        }
        else
        {
            mInterval = interval;
        }
    }

    auto IsRunning () -> bool { return !mIsDone && !mIsPaused; }
    auto IsPaused  () -> bool { return !mIsDone && mIsPaused; }
    auto IsStopped () -> bool { return mIsDone; }

};

} // namespace Caffeine
