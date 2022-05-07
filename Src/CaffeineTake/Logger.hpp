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

#include "Config.hpp"

#if defined(FEATURE_CAFFEINETAKE_LOGGER)

#include <spdlog/spdlog.h>

#include <filesystem>
#include <cstdarg>

namespace {
    namespace fs = std::filesystem;
}

// Logging macros.
#define LOG_TRACE(...)   spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...)   spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARNING(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)

namespace CaffeineTake {

auto InitLogger (const fs::path& logFilePath) -> bool;

} // namespace CaffeineTake

#else

// Logging macros.
#define LOG_TRACE(...)   do{}while(0)
#define LOG_DEBUG(...)   do{}while(0)
#define LOG_INFO(...)    do{}while(0)
#define LOG_WARNING(...) do{}while(0)
#define LOG_ERROR(...)   do{}while(0)

#endif // #if defined(FEATURE_CAFFEINETAKE_LOGGER)
