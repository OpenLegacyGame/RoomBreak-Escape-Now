#pragma once
#include <string_view>
namespace rb
{
// Publish a completed sibling temporary file without deleting the old save first.
bool replaceSaveFile(std::string_view temporary, std::string_view destination);
}  // namespace rb
