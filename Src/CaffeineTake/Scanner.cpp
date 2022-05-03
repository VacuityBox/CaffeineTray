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

#include "Scanner.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <optional>

#include <initguid.h>
#include <SetupAPI.h>
#include <usbiodef.h>
#include <bluetoothapis.h>

namespace CaffeineTake {

#pragma region "ProcessScanner"

auto ProcessScanner::CheckLast () -> bool
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

auto ProcessScanner::Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool
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
            const auto fileName = path.filename();
            for (const auto& procName : settings->Auto.ProcessNames)
            {
                if (procName == fileName)
                {
                    mLastProcessName = procName;
                    mLastPid         = pid;

                    spdlog::info(L"Found process: {} (PID: {})", procName, pid);
                    return ScanResult::Success;
                }
            }

            // Check if process is on process paths list.
            for (const auto& procPath : settings->Auto.ProcessPaths)
            {
                if (procPath == path)
                {
                    mLastProcessPath = procPath;
                    mLastPid         = pid;

                    spdlog::info(L"Found process: {} (PID: {})", procPath, pid);
                    return ScanResult::Success;
                }
            }

            if (stop)
            {
                return ScanResult::Stop;
            }

            return ScanResult::Continue;
        }
    );
}

#pragma endregion

#pragma region "WindowScanner"

auto WindowScanner::Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool
{
    if (settings->Auto.WindowTitles.empty())
    {
        return false;
    }

    return ScanWindows(
        [&](HWND hWnd, DWORD pid, std::wstring_view window)
        {
            // Check if process is on window title list.
            for (const auto& windowTitle : settings->Auto.WindowTitles)
            {
                if (windowTitle == window)
                {
                    spdlog::info(L"Found window: {} (PID: {})", windowTitle, pid);
                    return ScanResult::Success;
                }
            }

            if (stop)
            {
                return ScanResult::Stop;
            }

            return ScanResult::Continue;
        }
    );
}

#pragma endregion

#pragma region "UsbDeviceScanenr"

auto UsbDeviceScanner::Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool
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

    auto stopped = false;
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

            if (stop)
            {
                stopped = true;
                break;
            }
        }

        deviceIndex++;
    }

    // Cleanup.
    if (deviceInfoSet)
    {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }

    if (!stopped)
    {
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
    }

    return found;
}

#pragma endregion

#pragma region "BluetoothScanner"

auto BluetoothScanner::SystemTimeToChronoLocalTimePoint (const SYSTEMTIME& st)
{
    auto ft     = FILETIME{};
    auto ft_utc = FILETIME{};

    if (SystemTimeToFileTime(&st, &ft))
    {
        if (LocalFileTimeToFileTime(&ft, &ft_utc))
        {
            const auto tz = std::chrono::current_zone();
            const auto stp = FILETIME_to_system_clock(ft_utc);

            return tz->to_local(stp);
        }
    }

    return std::chrono::local_time<std::chrono::system_clock::duration>();
}

auto BluetoothScanner::ShouldPerformDeviceInquiry (const LocalTime& localTime, const std::chrono::seconds deviceActiveTimeout) -> bool
{
    auto issueInquiry = true;

    // If last inquiry was perfomed in less than mInquiryTimeout, skip.
    const auto diff = localTime - mLastInquiryTime;
    if (diff.count() > 0 && diff < mInquiryTimeout)
    {
        issueInquiry = false;                
    }
    else
    {
        if (mLastSeenMap.empty())
        {
            issueInquiry = false;
        }
        else
        {
            // Check if any device was last seen in deviceActiveTimeout.
            for (const auto& [id, lastSeen] : mLastSeenMap)
            {
                const auto diff = localTime - lastSeen;
                if (diff.count() > 0 && diff < deviceActiveTimeout)
                {
                    issueInquiry = false;
                    break;
                }
            }
        }
    }

    return issueInquiry;
}

auto BluetoothScanner::IssueDeviceInquiry () -> bool
{
    spdlog::trace("Starting bluetooth device inquiry");

    auto result = true;

    auto deviceInfo = BLUETOOTH_DEVICE_INFO{};
    ZeroMemory(&deviceInfo, sizeof(deviceInfo));
    deviceInfo.dwSize = sizeof(deviceInfo);

    auto searchParams = BLUETOOTH_DEVICE_SEARCH_PARAMS{
        .dwSize               = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
        .fReturnAuthenticated = TRUE,
        .fReturnRemembered    = TRUE,
        .fReturnUnknown       = TRUE,
        .fReturnConnected     = TRUE,
        .fIssueInquiry        = TRUE,
        .cTimeoutMultiplier   = 1,    // n*1.28s
        .hRadio               = NULL  // use all radios, for inquiry
    };

    auto deviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (deviceFind == NULL)
    {
        auto error = GetLastError();
        if (error != ERROR_NO_MORE_ITEMS)
        {
            spdlog::error("IssueDeviceInquiry() failed with error {}", GetLastError());
            result = false;
        }
        else
        {
            spdlog::debug("IssueDeviceInquiry() no more items");
        }
    }
    else
    {
        BluetoothFindDeviceClose(deviceFind);
    }

    spdlog::trace("Finished bluetooth device inqury");

    return result;
}

auto BluetoothScanner::CheckIfThereIsBluetoothRadio () -> bool
{
    auto params = BLUETOOTH_FIND_RADIO_PARAMS{
        .dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS)
    };

    auto found = false;

    auto radio = INVALID_HANDLE_VALUE;
    auto hRadioFind = BluetoothFindFirstRadio(&params, &radio);
    if (hRadioFind)
    {
        BluetoothFindRadioClose(hRadioFind);
        found = true;
    }

    return found;
}

auto BluetoothScanner::EnumerateBluetoothDevices (
    SettingsPtr                settings,
    const LocalTime&           localTime,
    const std::chrono::seconds deviceActiveTimeout,
    const StopToken&           stop
) -> BluetoothIdentifier
{
    auto deviceInfo = BLUETOOTH_DEVICE_INFO{};
    ZeroMemory(&deviceInfo, sizeof(deviceInfo));
    deviceInfo.dwSize = sizeof(deviceInfo);

    auto searchParams = BLUETOOTH_DEVICE_SEARCH_PARAMS{
        .dwSize               = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
        .fReturnAuthenticated = TRUE,
        .fReturnRemembered    = TRUE,
        .fReturnUnknown       = TRUE,
        .fReturnConnected     = TRUE,
        .fIssueInquiry        = FALSE,
        .cTimeoutMultiplier   = 0,
        .hRadio               = NULL  // use all radios, for inquiry
    };

    auto found = BluetoothIdentifier();
    auto deviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (deviceFind == NULL)
    {
        auto error = GetLastError();
        if (error != ERROR_NO_MORE_ITEMS)
        {
            spdlog::error("BluetoothFindFirstDevice() failed with error {}", GetLastError());
        }
        else
        {
            spdlog::debug("BluetoothFindFirstDevice() no more items");
        }
    }
    else
    {
        do
        {
            // Check if device is in the trigger list.
            for (const auto& id : settings->Auto.BluetoothDevices)
            {
                if (id == deviceInfo.Address.ullLong)
                {
                    // If device was seen in last mTimeoutDuration we consider it connected.
                    // If device wasn't seen in last mTimeoutDuration we will issue inquiry in next run*.
                    // * unless there is other device connected or seen in last mTimeoutDuration
                        
                    // Update last seen. deviceInfo.stLastSeen is expected to be in local time.
                    const auto lastSeen = SystemTimeToChronoLocalTimePoint(deviceInfo.stLastSeen);
                    mLastSeenMap[id.ull] = lastSeen;

                    const auto uniqid = id.ToWString();

                    if (deviceInfo.fConnected)
                    {
                        found = id;

                        if (mLastFoundDevice != id)
                        {
                            const auto name = std::wstring(deviceInfo.szName);
                            spdlog::info(L"Found connected Bluetooth device '{}' ({})", uniqid, name);
                            mLastFoundDevice = id;
                        }
                            
                        // Stop iterating.
                        break;
                    }
                    else
                    {
                        const auto diff = std::chrono::duration_cast<std::chrono::seconds>(localTime - lastSeen);
                        if (diff < deviceActiveTimeout)
                        {
                            if (found.IsInvalid())
                            {
                                found = id;

                                const auto name = std::wstring(deviceInfo.szName);
                                const auto diffStr = std::format(L"{}", diff);

                                if (found != mLastFoundDevice)
                                {
                                    spdlog::info(L"Bluetooth device '{}' ({}) was last seen in {}", uniqid, name, diffStr);
                                }
                            }
                        }

                        // Continue!
                    }
                }
            }

            if (stop)
            {
                break;
            }
        } while (BluetoothFindNextDevice(deviceFind, &deviceInfo));
        
        BluetoothFindDeviceClose(deviceFind);
    }

    return found;
}

auto BluetoothScanner::Run (SettingsPtr settings, const StopToken& stop, const PauseToken& pause) -> bool
{
    if (settings->Auto.BluetoothDevices.empty())
    {
        return false;
    }

    // Check if there is bluetooth adapter.
    if (!CheckIfThereIsBluetoothRadio())
    {
        spdlog::debug("No Bluetooth adapter found");
        return false;
    }

    // For some reason system keeps loading/unloading this library.
    // Load manually to keep at least one reference.
    if (!mLibBluetoothApis)
    {
        mLibBluetoothApis = LoadLibraryW(L"bluetoothapis.dll");
    }

    const auto deviceActiveTimeout = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::milliseconds(settings->Auto.ActiveTimeout)
    );

    const auto tz = std::chrono::current_zone();
    const auto localTime = tz->to_local(std::chrono::system_clock::now());

    // If we see didn't see at least one device in last mTimeoutDuration, issue inquiry.
    if (ShouldPerformDeviceInquiry(localTime, deviceActiveTimeout))
    {
        if (IssueDeviceInquiry())
        {
            spdlog::info("Finished Bluetooth device inquiry");
            mLastInquiryTime = localTime;
        }
    }

    // Enumerate bluetooth devices.
    auto found = EnumerateBluetoothDevices(settings, localTime, deviceActiveTimeout, stop);

    if (found.IsInvalid() && mLastFoundDevice.IsValid())
    {
        spdlog::info(L"Bluetooth device '{}' is no longer connected", mLastFoundDevice.ToWString());
    }

    if (found != mLastFoundDevice)
    {
        mLastFoundDevice = found;
    }

    return found.IsValid();
}

#pragma endregion

} // namespace CaffeineTake
