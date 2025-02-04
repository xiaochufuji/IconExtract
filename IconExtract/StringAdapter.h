#ifndef _STRINGADAPTER_H
#define _STRINGADAPTER_H
#include <string>
#include <filesystem>

namespace xiaochufuji
{
#if defined(UNICODE) || defined(_UNICODE) 
	// use unicode set
#define _string std::wstring
#define _char wchar_t
#define _sizec (sizeof(wchar_t))
#define _dot L"."
#define _slash L"/"
#define _connect_sign L"_"
#define _to_string std::to_wstring
#define _icoSuffix L".ico"
#define _bmpSuffix L".bmp"
#define PRINT std::wcout 
#else
#define _string std::string
#define _char char
#define _sizec (sizeof(char))
#define _dot "."
#define _slash "/"
#define _connect_sign "_"
#define _to_string std::to_string
#define _icoSuffix ".ico"
#define _bmpSuffix ".bmp"
#define PRINT std::cout 
#endif

	std::wstring StringToWString(const std::string& str);
	std::string WStringToString(const std::wstring& wstr);
	namespace fs = std::filesystem;
	bool CreateDirectoryIfNotExists(const fs::path& path);
}
#endif
