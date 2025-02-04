#pragma once
#include "StringAdapter.h"
#include <memory>
#include <windows.h>
#include <vector>
#include <gdiplus.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#pragma comment(lib, "gdiplus.lib")
namespace xiaochufuji
{
	enum class IconFormat : char{ PNG, BMP };
	struct HICONRAII {
		HICONRAII() = default;
		explicit HICONRAII(const HICON& icon) : icon(icon) {}
		~HICONRAII() { reset(); }
		HICONRAII(const HICONRAII&) = delete;
		HICONRAII& operator=(const HICONRAII&) = delete;
		HICONRAII(HICONRAII&& other) noexcept : icon(other.icon) {other.icon = nullptr;}
		HICONRAII& operator=(HICONRAII&& other) noexcept {
			if (this != &other) {
				reset();
				icon = other.icon;
				other.icon = nullptr;
			}
			return *this;
		}
		void reset() {
			if (icon) {
				DestroyIcon(icon);
				icon = nullptr;
			}
		}
		HICON icon{ nullptr };
	};

	struct GDIRAII {
		explicit GDIRAII() {
			Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiPlusStartupInput, NULL);
			if (status != Gdiplus::Ok) {
				throw std::runtime_error("GDI+ 初始化失败");
			}
			ok = true;
		}
		~GDIRAII() {
			if (gdiplusToken != 0) {
				Gdiplus::GdiplusShutdown(gdiplusToken);
				ok = false;
			}
		}
		GDIRAII(const GDIRAII&) = delete;
		GDIRAII& operator=(const GDIRAII&) = delete;
		GDIRAII(GDIRAII&&) = delete;
		GDIRAII& operator=(GDIRAII&&) = delete;
	public:
		ULONG_PTR gdiplusToken{};
		Gdiplus::GdiplusStartupInput gdiPlusStartupInput{};
		bool ok{ false };
	};
	struct BITMAPRAII {
		explicit BITMAPRAII(const HBITMAP& hBmp) : hBmp(hBmp) {
			if (hBmp != nullptr) {
				GetObject(hBmp, sizeof(BITMAP), &bmp);
			}
		}
		~BITMAPRAII() {
			if (hBmp != nullptr) {
				DeleteObject(hBmp);
				hBmp = nullptr;
			}
		}
		BITMAP bmp{};
		HBITMAP hBmp{ nullptr };
	};
#pragma pack(push, 1)
	typedef struct
	{
		WORD idReserved;
		WORD idType; // 1 = ICON, 2 = CURSOR
		WORD idCount; // number of images (and ICONDIRs)
	} IconDir; // file header           
	typedef struct
	{
		BYTE bWidth;
		BYTE bHeight;
		BYTE bColorCount;
		BYTE bReserved;
		WORD wPlanes;		// for cursors, this field = wXHotSpot
		WORD wBitCount;		// for cursors, this field = wYHotSpot
		DWORD dwBytesInRes;
		DWORD dwImageOffset; // file-offset to the start of ICONIMAGE
	} IconDirEntry;	// n directory entries
	typedef struct
	{
		BITMAPINFOHEADER biHeader; // header for color bitmap (no mask header)
		RGBQUAD rgbColors[1];
		BYTE bXOR[1]; // DIB bits for color bitmap
		BYTE bAND[1]; // DIB bits for mask bitmap
	} IconData;
#pragma pack(pop)
	class IconExtract
	{
	public:
		explicit IconExtract(const _string& filePath, UINT x, UINT y = 0);
		~IconExtract();
		int reload(const _char* filePath, UINT x, UINT y = 0);
		int saveAll(const _string& filePrefix, IconFormat format = IconFormat::PNG);
		int saveFirst(const _string& filePrefix, IconFormat format = IconFormat::PNG);

#pragma region PNG
	private:
		std::unique_ptr<Gdiplus::Bitmap> hiconConvertGdiBitmap(HICON hIcon);
		int GetEncoderClsid(CLSID& pClsid, const WCHAR* format = L"image/png");
		std::vector<BYTE> gdiBitmapConvertPngMemoryStream(const std::unique_ptr<Gdiplus::Bitmap>& bitmap);
		UINT writeIconDir(std::ofstream& iconFile, int nImage = 1);
		UINT writeIconDirEntry(std::ofstream& icoFile, BYTE bmpWidth, BYTE bmpHeight, DWORD pngSize, int index);
		UINT writeIconData(std::ofstream& icoFile, const std::vector<BYTE>& pngData);
		bool saveIconPng(HICON hIcon, const _string& dstFile, int nNumIcons = 1);
#pragma endregion
#pragma region BMP
	private:
		UINT writeIconDir(HANDLE hFile, int nImages);
		UINT numBitmapBytes(BITMAP* pBitmap);
		UINT writeIconImageHeader(HANDLE hFile, BITMAP* pbmpColor, BITMAP* pbmpMask);
		BOOL getIconBitmapInfo(HICON hIcon, ICONINFO* pIconInfo, BITMAP* pbmpColor, BITMAP* pbmpMask);
		UINT writeIconDirEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset);
		UINT writeIconData(HANDLE hFile, HBITMAP hBitmap);
		bool saveIconBmp(HICON hIcon, const _string& dstFile, int nNumIcons = 1);
#pragma endregion
	private:
		std::vector<HICONRAII> m_iconHandlers;
		std::unique_ptr<GDIRAII> m_gdiPlus = std::make_unique<GDIRAII>();
	};

};