#pragma once
#include <string>
#include <filesystem>

class FPaths
{
public:
    static std::string LoadFileToString(const std::string& FilePath);
    static std::string GetBinsDirectory();
    static std::string GetProjectDirectory();
    static std::string GetShaderDirectory();
    static bool DirectoryExists(const std::string& Directory);
    static bool FileExists(const std::string& FilePath);
    static std::string GetFileName(const std::string& FilePath);

    /*template <typename... Strings>
    static std::string Combine(Strings&&... strings);*/
};

/*
template <typename ... Strings>
std::string FPaths::Combine(Strings&&... strings)
{
    std::filesystem::path result;
    // Fold expression to combine all string arguments
    ((result /= std::filesystem::path(strings).relative_path()), ...);
    return result.lexically_normal().string();
}
*/
