#include "SaveStorage.h"
#include <filesystem>
#include <string>
#ifdef _WIN32
#    include <windows.h>
#endif
namespace rb
{
bool replaceSaveFile(std::string_view temporary, std::string_view destination)
{
    const auto source = std::filesystem::u8path(temporary.begin(), temporary.end());
    const auto target = std::filesystem::u8path(destination.begin(), destination.end());
#ifdef _WIN32
    return MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code error;
    std::filesystem::rename(source, target, error);
    return !error;
#endif
}
}  // namespace rb
