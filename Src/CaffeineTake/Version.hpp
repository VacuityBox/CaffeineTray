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
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <typeinfo>
#include <string_view>

namespace {
    using namespace std::literals;
}

namespace CaffeineTake {

constexpr auto CAFFEINE_TAKE_PROGRAM_NAME               = L"CaffeineTake"sv;
constexpr auto CAFFEINE_TAKE_VERSION_STRING             = L"3.0.0-dev"sv;
constexpr auto CAFFEINE_TAKE_LOG_FILENAME               = L"CaffeineTake.log"sv;
constexpr auto CAFFEINE_TAKE_SETTINGS_FILENAME          = L"CaffeineTake.json"sv;
constexpr auto CAFFEINE_TAKE_PORTABLE_SETTINGS_FILENAME = L"CaffeineTake.Portable.json"sv;

constexpr auto CAFFEINE_TAKE_AUTHOR  = L"Copyright (c) 2020-2021 VacuityBox"sv;
constexpr auto CAFFEINE_TAKE_LICENSE = std::wstring_view(
    L"This program is free software: you can redistribute it and/or modify\r\n"
    L"it under the terms of the GNU General Public License as published by\r\n"
    L"the Free Software Foundation, either version 3 of the License, or\r\n"
    L"(at your option) any later version.\r\n"
    L"\r\n"
    L"This program is distributed in the hope that it will be useful,\r\n"
    L"but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"
    L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\r\n"
    L"GNU General Public License for more details.\r\n"
    L"\r\n"
    L"You should have received a copy of the GNU General Public License\r\n"
    L"along with this program.  If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>\r\n"
);

constexpr auto CAFFEINE_TAKE_HOMEPAGE = std::wstring_view(
    L"<a href=\"https://github.com/serverfailure71/CaffeineTake\">https://github.com/serverfailure71/CaffeineTake/</a>"
);

constexpr auto THIRDPARTY_LICENSES = std::wstring_view(
    L"This program uses following 3rd party libraries:\r\n"
    L"\r\n"
    L"\r\n"
    L" JSON for Modern C++ version 3.9.1\r\n"
    L" Copyright (c) 2013-2021 Niels Lohmann <<a href=\"http://nlohmann.me\">http://nlohmann.me</a>>.\r\n"
    L" <a href=\"https://github.com/nlohmann/json\">https://github.com/nlohmann/json</a>\r\n"
    L"\r\n"
    L" Licensed under the MIT License <<a href=\"http://opensource.org/licenses/MIT\">http://opensource.org/licenses/MIT</a>>.\r\n"
    L" SPDX-License-Identifier: MIT\r\n"
);

} // namespace CaffeineTake
