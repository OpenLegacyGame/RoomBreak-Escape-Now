#if defined(RB_SAVE_STORAGE_STANDALONE)
#    include "SaveStorage.h"
#    include <filesystem>
#    include <fstream>
#    include <iostream>
#    include <stdexcept>
#    include <string>
namespace fs = std::filesystem;
std::string utf8(const fs::path& p)
{
    auto s = p.u8string();
    return {s.begin(), s.end()};
}
std::string read(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
void require(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    if (argc != 2)
        throw std::runtime_error("Provide an empty temporary test directory");
    auto root = fs::u8path(argv[1]);
    require(!fs::exists(root), "Test directory already exists");
    fs::create_directories(root);
    auto save = root / fs::u8path("游戏进度.json"), tmp = root / fs::u8path("游戏进度.json.tmp");
    std::ofstream(tmp) << "first";
    require(rb::replaceSaveFile(utf8(tmp), utf8(save)), "First save failed");
    require(read(save) == "first" && !fs::exists(tmp), "First save contents wrong");
    std::ofstream(tmp) << "second";
    require(rb::replaceSaveFile(utf8(tmp), utf8(save)), "Existing save replacement failed");
    require(read(save) == "second" && !fs::exists(tmp), "Replacement contents wrong");
    require(!rb::replaceSaveFile(utf8(tmp), utf8(save)), "Missing temporary file must fail");
    require(read(save) == "second", "Failed replacement destroyed previous save");
    auto blocked = root / "occupied";
    fs::create_directory(blocked);
    std::ofstream(blocked / "sentinel") << "keep";
    std::ofstream(tmp) << "third";
    require(!rb::replaceSaveFile(utf8(tmp), utf8(blocked)), "Directory destination must fail");
    require(read(tmp) == "third" && read(blocked / "sentinel") == "keep" && read(save) == "second",
            "Failure modified saved files");
    fs::remove_all(root);
    std::cout << "PASS: new save, replacement, Unicode path, missing source and blocked destination preserve data\n";
}

#endif
