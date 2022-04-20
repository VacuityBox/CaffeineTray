#pragma once

#include <filesystem>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

auto InitLogger (const fs::path& logFilePath) -> bool;

} // namespace Caffeine
