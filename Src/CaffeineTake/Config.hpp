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

// ====================================================== //
// Comment/Uncomment to Enable/Disable specific features. //
// ====================================================== //

#define ENABLE_FEATURE_LOGGER
#define ENABLE_FEATURE_AUTO_MODE
#define ENABLE_FEATURE_TIMER_MODE
#define ENABLE_FEATURE_AUTO_MODE_TRIGGER_PROCESS
#define ENABLE_FEATURE_AUTO_MODE_TRIGGER_WINDOW
#define ENABLE_FEATURE_AUTO_MODE_TRIGGER_USB
#define ENABLE_FEATURE_AUTO_MODE_TRIGGER_BLUETOOTH
#define ENABLE_FEATURE_AUTO_MODE_TRIGGER_SCHEDULE
#define ENABLE_FEATURE_SETTINGS
#define ENABLE_FEATURE_IMMERSIVE_CONTEXT_MENU
#define ENABLE_FEATURE_JUMPLISTS
#define ENABLE_FEATURE_SETTINGS_DIALOG
#define ENABLE_FEATURE_CUSTOM_ICONS
#define ENABLE_FEATURE_MULTILANG

// ============================ //
// Don't modify anything below! //
//                              //
//    Feature Configurations    //
// ============================ //

#define FEATURE_SET_CUSTOM      0    // default
#define FEATURE_SET_MINIMAL     1
#define FEATURE_SET_STANDARD    2
#define FEATURE_SET_FULL        3

// To change feature configuration define FEATURE_SET from build system.
#if !defined(FEATURE_SET)
#   define FEATURE_SET FEATURE_TYPE_CUSTOM
#else
#   if (FEATURE_SET != FEATURE_SET_MINIMAL)  \
    && (FEATURE_SET != FEATURE_SET_STANDARD) \
    && (FEATURE_SET != FEATURE_SET_FULL)
#       error Invalid feature set specified
#   endif
#endif

// ========================== //
// Minimal Build              //
// ========================== //

#if FEATURE_SET == FEATURE_SET_MINIMAL
#   define FEATURE_SET_STRING "Minimal"
// ...
#endif

// ========================== //
// Standard Build             //
// ========================== //

#if FEATURE_SET == FEATURE_SET_STANDARD
#   define FEATURE_SET_STRING "Standard"
#   define FEATURE_CAFFEINETAKE_LOGGER
#   define FEATURE_CAFFEINETAKE_AUTO_MODE
#   define FEATURE_CAFFEINETAKE_TIMER_MODE
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE
#   define FEATURE_CAFFEINETAKE_SETTINGS
#   define FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU
#   define FEATURE_CAFFEINETAKE_JUMPLISTS
#   define FEATURE_CAFFEINETAKE_SETTINGS_DIALOG
#   define FEATURE_CAFFEINETAKE_CUSTOM_ICONS
#   define FEATURE_CAFFEINETAKE_MULTILANG
#endif

// ========================== //
// Full Build                 //
// ========================== //

#if FEATURE_SET == FEATURE_SET_FULL
#   define FEATURE_SET_STRING "Full"
#   define FEATURE_CAFFEINETAKE_LOGGER
#   define FEATURE_CAFFEINETAKE_AUTO_MODE
#   define FEATURE_CAFFEINETAKE_TIMER_MODE
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH
#   define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE
#   define FEATURE_CAFFEINETAKE_SETTINGS
#   define FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU
#   define FEATURE_CAFFEINETAKE_JUMPLISTS
#   define FEATURE_CAFFEINETAKE_SETTINGS_DIALOG
#   define FEATURE_CAFFEINETAKE_CUSTOM_ICONS
#   define FEATURE_CAFFEINETAKE_MULTILANG
#endif

// ========================== //
// Custom Build               //
// ========================== //

#if FEATURE_SET == FEATURE_SET_CUSTOM

#define FEATURE_SET_STRING "Custom"

// Logger.
#if defined(ENABLE_FEATURE_LOGGER)
    #define FEATURE_CAFFEINETAKE_LOGGER
#endif

// Caffeine Auto Mode.
#if defined(ENABLE_FEATURE_AUTO_MODE)
#   define FEATURE_CAFFEINETAKE_AUTO_MODE

#   if defined (ENABLE_FEATURE_AUTO_MODE_TRIGGER_PROCESS)
#       define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_PROCESS
#   endif

#   if defined (ENABLE_FEATURE_AUTO_MODE_TRIGGER_WINDOW)
#       define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_WINDOW
#   endif

#   if defined (ENABLE_FEATURE_AUTO_MODE_TRIGGER_USB)
#       define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB
#   endif

#   if defined (ENABLE_FEATURE_AUTO_MODE_TRIGGER_BLUETOOTH)
#       define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH
#   endif

#   if defined (ENABLE_FEATURE_AUTO_MODE_TRIGGER_SCHEDULE)
#       define FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_SCHEDULE
#   endif
#endif

// Caffeine Timer Mode.
#if defined(ENABLE_FEATURE_TIMER_MODE)
#   define FEATURE_CAFFEINETAKE_TIMER_MODE
#endif

// Settings.
#if defined(ENABLE_FEATURE_SETTINGS)
#   define FEATURE_CAFFEINETAKE_SETTINGS
#endif

// Immersive Context Menu.
#if defined(ENABLE_FEATURE_IMMERSIVE_CONTEXT_MENU)
#   define FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU
#endif

// Jump Lists.
#if defined(ENABLE_FEATURE_JUMPLISTS)
#   define FEATURE_CAFFEINETAKE_JUMPLISTS
#endif

// Settings Dialog.
#if defined(ENABLE_FEATURE_SETTINGS_DIALOG)
#   define FEATURE_CAFFEINETAKE_SETTINGS_DIALOG
#endif

// Custom Icons Loading.
#if defined(ENABLE_FEATURE_CUSTOM_ICONS)
#   define FEATURE_CAFFEINETAKE_CUSTOM_ICONS
#endif

// Language Support.
#if defined(ENABLE_FEATURE_MULTILANG)
#   define FEATURE_CAFFEINETAKE_MULTILANG
#endif

#endif // #if FEATURE_SET == FEATURE_SET_CUSTOM

// ====== //
// Undefs //
// ====== //

#undef ENABLE_FEATURE_LOGGER
#undef ENABLE_FEATURE_AUTO_MODE
#undef ENABLE_FEATURE_TIMER_MODE
#undef ENABLE_FEATURE_AUTO_MODE_TRIGGER_PROCESS
#undef ENABLE_FEATURE_AUTO_MODE_TRIGGER_WINDOW
#undef ENABLE_FEATURE_AUTO_MODE_TRIGGER_USB
#undef ENABLE_FEATURE_AUTO_MODE_TRIGGER_BLUETOOTH
#undef ENABLE_FEATURE_AUTO_MODE_TRIGGER_SCHEDULE
#undef ENABLE_FEATURE_SETTINGS
#undef ENABLE_FEATURE_IMMERSIVE_CONTEXT_MENU
#undef ENABLE_FEATURE_JUMPLISTS
#undef ENABLE_FEATURE_SETTINGS_DIALOG
#undef ENABLE_FEATURE_CUSTOM_ICONS
#undef ENABLE_FEATURE_MULTILANG
