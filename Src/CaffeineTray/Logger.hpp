#pragma once

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <queue>
#include <utility>

namespace Caffeine {

class Logger final
{
    using time_point = std::chrono::system_clock::time_point;
    using msg_type   = std::pair<time_point, std::string>;

    std::queue<msg_type>    mMessageQueue;
    std::queue<msg_type>    mWorkerQueue;
    std::mutex              mQueueMutex;
    std::condition_variable mWorkerConditionVar;
    std::ofstream           mFile;
    std::thread             mWorkerThread;
    bool                    mIsDone;

    auto Worker () -> void;

    Logger            (const Logger& rhs) = delete;
    Logger& operator= (const Logger& rhs) = delete;

public:
    Logger ()
        : mWorkerThread (&Logger::Worker, this)
        , mIsDone       (false)
    {
    }

    ~Logger ()
    {
        mIsDone = true;
        mWorkerConditionVar.notify_one(); // notify in case thread is waiting on condition
        mWorkerThread.join();
    }

    auto Open (const std::filesystem::path path) -> bool
    {
        mFile.open(path);
        return mFile.is_open();
    }

    auto Log (std::string message) -> void;
};

} // namespace Caffeine
