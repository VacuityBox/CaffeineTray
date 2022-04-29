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

#include "Settings.hpp"
#include "Utility.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <Windows.h>
#include <initguid.h>
#include <SetupAPI.h>
#include <usbiodef.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class Scanner
{
public:
    virtual ~Scanner() {}

    virtual auto Run (SettingsPtr) -> bool = 0;
};

class ProcessScanner : public Scanner
{
    std::wstring mLastProcessName = L"";
    std::wstring mLastProcessPath = L"";
    DWORD        mLastPid         = 0;

    auto CheckLast () -> bool
    {
        auto path = GetProcessPath(mLastPid);
        if (!path.empty())
        {
            if (mLastProcessPath.empty())
            {
                return path.filename() == mLastProcessName;
            }
            else
            {
                return path == mLastProcessPath;
            }
        }

        return false;
    }

public:
    virtual auto Run (SettingsPtr settings) -> bool override
    {
        if (settings->Auto.ProcessNames.empty() && settings->Auto.ProcessPaths.empty())
        {
            return false;
        }

        // Only check last.
        if (mLastPid != 0)
        {
            if (CheckLast())
            {
                return true;
            }

            const auto& last = mLastProcessPath.empty() ? mLastProcessName : mLastProcessPath;
            spdlog::info(L"Process: {} (PID: {}), no longer exists, scanning all processes", last, mLastPid);
        }

        mLastProcessName.clear();
        mLastProcessPath.clear();
        mLastPid = 0;

        return ScanProcesses(
            [&](HANDLE handle, DWORD pid, fs::path path)
            {
                // Check if process is on process names list.
                for (auto procName : settings->Auto.ProcessNames)
                {
                    if (procName == path.filename())
                    {
                        mLastProcessName = procName;
                        mLastPid         = pid;

                        spdlog::info(L"Found process: {} (PID: {})", procName, pid);
                        return true;
                    }
                }

                // Check if process is on process paths list.
                for (auto procPath : settings->Auto.ProcessPaths)
                {
                    if (procPath == path)
                    {
                        mLastProcessPath = procPath;
                        mLastPid         = pid;

                        spdlog::info(L"Found process: {} (PID: {})", procPath, pid);
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

class WindowScanner : public Scanner
{
public:
    auto Run (SettingsPtr settings) -> bool override
    {
        if (settings->Auto.WindowTitles.empty())
        {
            return false;
        }

        return ScanWindows(
            [&](HWND hWnd, DWORD pid, std::wstring_view window)
            {
                // Check if process is on window title list.
                for (auto windowTitle : settings->Auto.WindowTitles)
                {
                    if (windowTitle == window)
                    {
                        spdlog::info(L"Found window: {} (PID: {})", windowTitle, pid);
                        return true;
                    }
                }

                return false; // don't stop iterating
            }
        );
    }
};

class UsbDeviceScanner : public Scanner
{
    std::wstring mLastFoundDevice = L"";

public:
    auto Run (SettingsPtr settings) -> bool override
    {
        if (settings->Auto.UsbDevices.empty())
        {
            return false;
        }

        // Get list of USB devices that are present in the system.
        auto deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        if (deviceInfoSet == INVALID_HANDLE_VALUE)
        {
            return false;
        }
    
        auto deviceIndex = DWORD{0};
        auto deviceInfoData = SP_DEVINFO_DATA{};
        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        auto found = false;
        auto buffer = std::vector<wchar_t>();
        buffer.resize(1024);

        auto error = DWORD{0};
        while (error != ERROR_NO_MORE_ITEMS)
        {
            auto success = SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData);
            if (!success)
            {
                error = GetLastError();

                if (error != ERROR_NO_MORE_ITEMS)
                {
                    spdlog::error("SetupDiEnumDeviceInfo() failed with error: {}", error);
                    break;
                }
            }
            else
            {
                // Get required buffer size.
                auto requiredSize = DWORD{0};
                success = SetupDiGetDeviceInstanceIdW(deviceInfoSet, &deviceInfoData, NULL, 0, &requiredSize);
                if (!success)
                {
                    error = GetLastError();

                    if (error != ERROR_INSUFFICIENT_BUFFER)
                    {
                        spdlog::error("SetupDiGetDeviceInstanceIdW() failed with error: {}", error);
                        break;
                    }
                }

                // Resize buffer if needed.
                if (requiredSize > buffer.capacity())
                {
                    buffer.resize(requiredSize);
                }

                // Get device id.
                success = SetupDiGetDeviceInstanceIdW(deviceInfoSet, &deviceInfoData, buffer.data(), buffer.capacity(), &requiredSize);
                if (!success)
                {
                    error = GetLastError();

                    spdlog::error("SetupDiGetDeviceInstanceIdW() failed with error: {}", error);
                    break;
                }

                // Check if device is in the trigger list.
                for (const auto& id : settings->Auto.UsbDevices)
                {
                    if (id == std::wstring_view(buffer.data()))
                    {
                        found = true;
                        break;
                    }
                }
            }

            deviceIndex++;
        }

        // Cleanup.
        if (deviceInfoSet)
        {
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
        }

        if (found)
        {
            if (mLastFoundDevice != std::wstring_view(buffer.data()))
            {
                mLastFoundDevice = buffer.data();
                spdlog::info(L"Found present USB device: '{}'", mLastFoundDevice);
            }
        }
        else
        {
            if (!mLastFoundDevice.empty())
            {
                spdlog::info(L"USB Device '{}' is no longer present in system", mLastFoundDevice);
            }

            mLastFoundDevice = L"";
        }

        return found;
    }
};

} // namespace CaffeineTake
