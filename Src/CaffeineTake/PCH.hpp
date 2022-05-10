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

//#include "Config.hpp"
//
//// Modern NotifyIcon
//#if defined(FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU)
//    #include <mni/ImmersiveNotifyIcon.hpp>
//#else
//    #include <mni/ClassicNotifyIcon.hpp>
//#endif

// nlohmann json
#include <nlohmann/json.hpp>

// spdlog
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

// C++ Library
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <string>
#include <thread>
#include <typeinfo>
#include <utility>
#include <vector>

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT_TYPED_ITEMIDS

#include <Windows.h>
#include <windowsx.h>
#include <initguid.h>

#include <CommCtrl.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <VersionHelpers.h>

// USB 
#include <SetupAPI.h>
#include <usbiodef.h>

// Bluetooth
#include <bluetoothapis.h>

// Jump Lists
#include <knownfolders.h>
#include <objectarray.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shobjidl.h>

// Lockscreen Detection
#include <WtsApi32.h>
