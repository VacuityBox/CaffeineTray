#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Caffeine {

auto UTF8ToUTF16 (const std::string_view str) -> std::optional<std::wstring>;
auto UTF16ToUTF8 (const std::wstring_view str) -> std::optional<std::string>;

auto GetAppDataPath () -> std::filesystem::path;
auto IsLightTheme   () -> bool;

} // namespace Caffeine
