#include "Paths.h"
#include <codecvt>
#include <fstream>
#include <locale>
#include <sstream>
#include <filesystem> 
#include "VulkanoLog.h"


std::string FPaths::LoadFileToString(const std::string& FilePath)
{
    std::ifstream file(FilePath);
    if (!file.is_open())
    {
        VK_LOG(LOG_WARNING, "Could not opend file %s", FilePath.c_str());
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string FPaths::GetBinsDirectory()
{
    std::filesystem::path exePath = std::filesystem::current_path();
    std::string path = exePath.string();
    std::replace(path.begin(), path.end(), '\\', '/');
    return path + "/Vulkano";
}

std::string FPaths::GetProjectDirectory()
{
    std::string path = GetBinsDirectory();
    std::size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        // Return the substring up to the last backslash
        return path.substr(0, pos);
    }
    return path; 
}

std::string FPaths::GetShaderDirectory()
{
    return GetProjectDirectory() + "/Shaders";
}

bool FPaths::DirectoryExists(const std::string& Directory)
{
    return std::filesystem::exists(Directory) && std::filesystem::is_directory(Directory);
}

bool FPaths::FileExists(const std::string& FilePath)
{
    return std::filesystem::exists(FilePath) && std::filesystem::is_regular_file(FilePath);
}

std::string FPaths::GetFileName(const std::string& FilePath)
{
    std::filesystem::path filePath(FilePath);
    return filePath.filename().string();
}
