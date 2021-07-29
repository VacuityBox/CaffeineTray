#pragma once

#include "Settings.hpp"
#include "Utility.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

class Scanner
{
public:
    virtual ~Scanner() {}

    virtual auto Run () -> bool = 0;
};

class ProcessScanner : public Scanner
{
    std::shared_ptr<Settings> mSettingsPtr;
    std::wstring              mLastFound;

public:
    ProcessScanner (std::shared_ptr<Settings> settings)
        : mSettingsPtr (settings)
    {
    }

    auto Run () -> bool override
    {
        mLastFound = L"";
        return ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                // Check if process is on process names list.
                for (auto procName : mSettingsPtr->Auto.ProcessNames)
                {
                    if (procName == path.filename())
                    {
                        mLastFound = procName;
                        return true;
                    }
                }

                // Check if process is on process paths list.
                for (auto procPath : mSettingsPtr->Auto.ProcessPaths)
                {
                    if (procPath == path)
                    {
                        mLastFound = procPath;
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }

    const auto& GetLastFound() { return mLastFound; }
};

class WindowScanner : public Scanner
{
    std::shared_ptr<Settings> mSettingsPtr;
    std::wstring              mLastFound;

public:
    WindowScanner (std::shared_ptr<Settings> settings)
        : mSettingsPtr (settings)
    {
    }

    auto Run () -> bool override
    {
        mLastFound = L"";
        return ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view window)
            {
                // Check if process is on window title list.
                for (auto windowTitle : mSettingsPtr->Auto.WindowTitles)
                {
                    if (windowTitle == window)
                    {
                        mLastFound = window;
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }

    const auto& GetLastFound() { return mLastFound; }
};

} // namespace Caffeine
