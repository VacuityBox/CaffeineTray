#pragma once

#include "IconCache.hpp"
#include "Utility.hpp"

#include <string>
#include <utility>
#include <vector>

namespace Caffeine {

struct ProcessInfo
{
    std::wstring path;
    std::wstring name;
    std::wstring window;
    int          icon;

    ProcessInfo (std::wstring path, std::wstring name, std::wstring window, int icon)
        : path   (path)
        , name   (name)
        , window (window)
        , icon   (icon)
    {
    }
};

class RunningProcessList
{
    std::vector<std::pair<int, ProcessInfo>> mRunningProcesses;
    std::shared_ptr<IconCache>               mIconCache;

public:
    RunningProcessList (std::shared_ptr<IconCache> iconCache)
        : mIconCache (iconCache)
    {
    }

          auto& Get ()       { return mRunningProcesses; }
    const auto& Get () const { return mRunningProcesses; }

    auto GetIconCache () { return mIconCache; }

    auto Refresh () -> void
    {
        mRunningProcesses.clear();

        // Load list of running processes.
        ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                auto icon = mIconCache->Insert(path);
                mRunningProcesses.push_back(
                    std::make_pair(
                        pid,
                        ProcessInfo(path.wstring(), path.filename().wstring(), std::wstring(), icon)
                    )
                );

                return false; // don't stop iterating
            }
        );

        // Load window titles.
        ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view title)
            {
                for (auto& process : mRunningProcesses)
                {
                    if (process.first == pid)
                    {
                        process.second.window = title;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

} // namespace Caffeine
