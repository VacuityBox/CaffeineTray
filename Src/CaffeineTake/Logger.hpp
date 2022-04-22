#pragma once

#include <filesystem>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

auto InitLogger (const fs::path& logFilePath) -> bool;

} // namespace CaffeineTake
