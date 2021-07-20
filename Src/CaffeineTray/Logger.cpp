#include "Logger.hpp"

#include <array>

namespace Caffeine {

auto Logger::Worker () -> void
{
    const auto format  = "%F %T";
    const auto maxSize = 32;

    auto buffer    = std::array<char, maxSize>{ 0 };
    auto queueLock = std::unique_lock<std::mutex>(mQueueMutex); // this locks
        
    while (!mIsDone || !mMessageQueue.empty())
    {
        // Wait until something appear on queue or when thread need to end.
        mWorkerConditionVar.wait(queueLock, 
            [&]
            {
                return !mMessageQueue.empty() || mIsDone;
            }
        );

        // If queue is empty this means mIsDone was set to true. We can just return.
        if (mMessageQueue.empty())
        {
            continue;
        }

        std::swap(mMessageQueue, mWorkerQueue);
            
        // We can unlock queue mutex for file write.
        queueLock.unlock();

        // Write messages to file.
        while (!mWorkerQueue.empty())
        {
            const auto [logTime, message] = mWorkerQueue.front();
            mWorkerQueue.pop();

            const auto time = std::chrono::system_clock::to_time_t(logTime);
            auto tm = std::tm();
            const auto err = localtime_s(&tm, &time);

            buffer.fill('\0');
            const auto size = std::strftime(buffer.data(), buffer.size() - 1, format, &tm);

            mFile << "[" << buffer.data() << "] " << message.data() << "\n";
        }
            
        mFile.flush();

        // Lock mutex before loop check.
        queueLock.lock();
    }
}

auto Logger::Log (std::string message) -> void
{
    if (!mFile)
    {
        return;
    }

    // Get message log time.
    auto now = std::chrono::system_clock::now();

    // Push message to queue.
    mQueueMutex.lock();
        
    const auto wasEmpty = mMessageQueue.empty();
    mMessageQueue.emplace(std::make_pair(std::move(now), std::move(message)));
        
    mQueueMutex.unlock();
        
    // Notify the worker to do condition check on wait().
    if (wasEmpty)
    {
        mWorkerConditionVar.notify_one();
    }
}

} // namespace Caffeine
