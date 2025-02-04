#include "StringAdapter.h"
#include <Windows.h>
#include <filesystem>
std::wstring xiaochufuji::StringToWString(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

std::string xiaochufuji::WStringToString(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size_needed, nullptr, nullptr);
	return str;
}

bool xiaochufuji::CreateDirectoryIfNotExists(const fs::path& path)
{
	std::error_code ec;
    if (fs::exists(path, ec)) {
        return fs::is_directory(path, ec);
    }
    return fs::create_directories(path, ec);
}
