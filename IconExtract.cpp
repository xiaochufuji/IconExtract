#include "IconExtract.h"
#include <iostream>
#include <fstream>
#include <atlbase.h>
#include <string>

xiaochufuji::IconExtract::IconExtract(const _string& filePath, UINT x, UINT y)
{
	reload(filePath.c_str(), x, y);
}

xiaochufuji::IconExtract::~IconExtract()
{
}

int xiaochufuji::IconExtract::reload(const _char* filePath, UINT x, UINT y)
{
	y = (y == 0 || y != x) ? x : y;
	UINT iconCount = PrivateExtractIcons(filePath, 0, x, y, nullptr, nullptr, 0, 0);
	if (iconCount == 0xFFFFFFFF) {
		return -1;
	}
	if (iconCount == 0) {
		return 0;
	}

	std::vector<HICON> icons(iconCount, nullptr);
	m_iconHandlers.resize(iconCount);
	iconCount = PrivateExtractIcons(filePath, 0, x, y, icons.data(), nullptr, iconCount, 0);
	for (UINT i = 0; i < iconCount; ++i) {
		if (icons[i]) {
			m_iconHandlers[i] = std::move(HICONRAII(icons[i]));
		}
	}
	return iconCount;
}

int xiaochufuji::IconExtract::saveAll(const _string& filePrefix, IconFormat format)
{
	int count = 0;
	for (UINT i = 0; i < m_iconHandlers.size(); ++i) {
		if (m_iconHandlers[i].icon) {
			_string fileName = filePrefix + _connect_sign + _to_string(i) + _icoSuffix;
			bool success = false;
			if(format == IconFormat::PNG)
				success = saveIconPng(m_iconHandlers[i].icon, fileName);
			else 
				success = saveIconBmp(m_iconHandlers[i].icon, fileName);
			if (success) count++;
		}
	}
	return count;
}

int xiaochufuji::IconExtract::saveFirst(const _string& filePrefix, IconFormat format)
{
	if (m_iconHandlers.size() < 1) return 0;
	if (m_iconHandlers[0].icon) {
		_string fileName = filePrefix + _connect_sign + _icoSuffix;
		bool success = false;
		if (format == IconFormat::PNG)
			success = saveIconPng(m_iconHandlers[0].icon, fileName);
		else
			success = saveIconBmp(m_iconHandlers[0].icon, fileName);
		if (success) return 1;
	}
	return 0;
}

UINT xiaochufuji::IconExtract::writeIconDir(HANDLE hFile, int nImages)
{
	IconDir iconheader{};
	DWORD nWritten;
	iconheader.idReserved = 0; // Must be 0
	iconheader.idType = 1; // Type 1 = ICON (type 2 = CURSOR)
	iconheader.idCount = nImages;
	WriteFile(hFile, &iconheader, sizeof(iconheader), &nWritten, 0);
	return nWritten;
}

UINT xiaochufuji::IconExtract::numBitmapBytes(BITMAP* pBitmap)
{
	int nWidthBytes = pBitmap->bmWidthBytes;
	if (nWidthBytes & 3)
		nWidthBytes = (nWidthBytes + 4) & ~3;
	return nWidthBytes * pBitmap->bmHeight;
}

UINT xiaochufuji::IconExtract::writeIconImageHeader(HANDLE hFile, BITMAP* pbmpColor, BITMAP* pbmpMask)
{
	BITMAPINFOHEADER biHeader;
	DWORD nWritten;
	UINT nImageBytes;

	// calculate how much space the COLOR and MASK bitmaps take
	nImageBytes = numBitmapBytes(pbmpColor) + numBitmapBytes(pbmpMask);

	// write the ICONIMAGE to disk (first the BITMAPINFOHEADER)
	ZeroMemory(&biHeader, sizeof(biHeader));

	// Fill in only those fields that are necessary
	biHeader.biSize = sizeof(biHeader);
	biHeader.biWidth = pbmpColor->bmWidth;
	biHeader.biHeight = pbmpColor->bmHeight * 2; // height of color+mono
	biHeader.biPlanes = pbmpColor->bmPlanes;
	biHeader.biBitCount = pbmpColor->bmBitsPixel;
	biHeader.biSizeImage = nImageBytes;

	// write the BITMAPINFOHEADER
	WriteFile(hFile, &biHeader, sizeof(biHeader), &nWritten, 0);

	return nWritten;
}

BOOL xiaochufuji::IconExtract::getIconBitmapInfo(HICON hIcon, ICONINFO* pIconInfo, BITMAP* pbmpColor, BITMAP* pbmpMask)
{
	if (!GetIconInfo(hIcon, pIconInfo))
		return FALSE;

	if (!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
		return FALSE;

	if (!GetObject(pIconInfo->hbmMask, sizeof(BITMAP), pbmpMask))
		return FALSE;

	return TRUE;
}

UINT xiaochufuji::IconExtract::writeIconDirEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset)
{
	ICONINFO iconInfo;
	IconDirEntry iconDirEntry;
	BITMAPRAII bmpColor(iconInfo.hbmColor), bmpMask(iconInfo.hbmMask);

	DWORD nWritten;
	UINT nColorCount;
	UINT nImageBytes;

	nImageBytes = numBitmapBytes(&bmpColor.bmp) + numBitmapBytes(&bmpMask.bmp);

	if (bmpColor.bmp.bmBitsPixel >= 8)
		nColorCount = 0;
	else
		nColorCount = 1 << (bmpColor.bmp.bmBitsPixel * bmpColor.bmp.bmPlanes);

	iconDirEntry.bWidth = (BYTE)bmpColor.bmp.bmWidth;
	iconDirEntry.bHeight = (BYTE)bmpColor.bmp.bmHeight;
	iconDirEntry.bColorCount = nColorCount;
	iconDirEntry.bReserved = 0;
	iconDirEntry.wPlanes = bmpColor.bmp.bmPlanes;
	iconDirEntry.wBitCount = bmpColor.bmp.bmBitsPixel;
	iconDirEntry.dwBytesInRes = sizeof(BITMAPINFOHEADER) + nImageBytes;
	iconDirEntry.dwImageOffset = nImageOffset;

	WriteFile(hFile, &iconDirEntry, sizeof(iconDirEntry), &nWritten, 0);
	return nWritten;
}

UINT xiaochufuji::IconExtract::writeIconData(HANDLE hFile, HBITMAP hBitmap)
{
	BITMAPRAII bmp(hBitmap);
	UINT nBitmapBytes;
	DWORD nWritten;

	nBitmapBytes = numBitmapBytes(&bmp.bmp);

	std::unique_ptr<BYTE, void(*)(void*)> pIconData((BYTE*)malloc(nBitmapBytes)
		,[](void* p) {if (p)
	{
		free(p);
	}});

	GetBitmapBits(hBitmap, nBitmapBytes, pIconData.get());

	// bitmaps are stored inverted (vertically) when on disk..
	// so write out each line in turn, starting at the bottom + working
	// towards the top of the bitmap. Also, the bitmaps are stored in packed
	// in memory - scanlines are NOT 32bit aligned, just 1-after-the-other
	for (int i = bmp.bmp.bmHeight - 1; i >= 0; i--)
	{
		// Write the bitmap scanline
		WriteFile(
			hFile,
			pIconData.get() + (i * bmp.bmp.bmWidthBytes), // calculate offset to the line
			bmp.bmp.bmWidthBytes, // 1 line of BYTES
			&nWritten,
			0);

		// extend to a 32bit boundary (in the file) if necessary
		if (bmp.bmp.bmWidthBytes & 3)
		{
			DWORD padding = 0;
			WriteFile(hFile, &padding, 4 - bmp.bmp.bmWidthBytes, &nWritten, 0);
		}
	}
	return nBitmapBytes;
}

bool xiaochufuji::IconExtract::saveIconBmp(HICON hIcon, const _string& dstFile, int nNumIcons)
{
	HANDLE hFile;
	if (hIcon == 0 || nNumIcons < 1)
		return false;

	hFile = CreateFile(dstFile.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	writeIconDir(hFile, nNumIcons);

	SetFilePointer(hFile, sizeof(IconDirEntry) * nNumIcons, 0, FILE_CURRENT);
	std::vector<int> pImageOffset;
	pImageOffset.resize(nNumIcons, -1);

	for (int i = 0; i < nNumIcons; i++)
	{
		ICONINFO iconInfo;
		BITMAPRAII bmpColor(iconInfo.hbmColor), bmpMask(iconInfo.hbmMask);

		// record the file-offset of the icon image for when we write the icon directories
		pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

		// bitmapinfoheader + colortable
		writeIconImageHeader(hFile, &bmpColor.bmp, &bmpMask.bmp);

		// color and mask bitmaps
		writeIconData(hFile, iconInfo.hbmColor);
		writeIconData(hFile, iconInfo.hbmMask);
	}

	SetFilePointer(hFile, sizeof(IconDir), 0, FILE_BEGIN);

	for (int i = 0; i < nNumIcons; i++)
	{
		writeIconDirEntry(hFile, i, hIcon, pImageOffset[i]);
	}

	CloseHandle(hFile);
	return true;
}

std::unique_ptr<Gdiplus::Bitmap> xiaochufuji::IconExtract::hiconConvertGdiBitmap(HICON hIcon)
{
	ICONINFO iconInfo;
	if (!GetIconInfo(hIcon, &iconInfo)) {
		return nullptr;
	}

	BITMAPRAII bmpColor(iconInfo.hbmColor), bmpMask(iconInfo.hbmMask);
	auto colorWidth = bmpColor.bmp.bmWidth;
	auto colorHeight = bmpColor.bmp.bmHeight;
	auto colorWidthBytes = bmpColor.bmp.bmWidthBytes;
	auto maskWidth = bmpMask.bmp.bmWidth;
	auto maskHeight = bmpMask.bmp.bmHeight;
	auto maskWidthBytes = bmpMask.bmp.bmWidthBytes;

	// 创建带透明通道的 Bitmap 对象(空图像)
	std::unique_ptr<Gdiplus::Bitmap> bmp = std::make_unique<Gdiplus::Bitmap>(colorWidth, colorHeight, PixelFormat32bppARGB);
	Gdiplus::BitmapData bmpData;
	Gdiplus::Rect rect(0, 0, colorWidth, colorHeight);
	bmp->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmpData);

	// 提取颜色位图数据
	std::vector<BYTE> colorData(colorHeight * colorWidthBytes);
	GetBitmapBits(iconInfo.hbmColor, static_cast<LONG>(colorData.size()), colorData.data());

	// 提取遮罩位图数据
	std::vector<BYTE> maskData(maskHeight * maskWidthBytes);
	GetBitmapBits(iconInfo.hbmMask, static_cast<LONG>(maskData.size()), maskData.data());

	BYTE* pPixels = (BYTE*)bmpData.Scan0;
	int stride = bmpData.Stride;

	for (int y = 0; y < colorHeight; y++) {
		for (int x = 0; x < colorWidth; x++) {
			int pixelIndex = y * colorWidthBytes + x * 4;
			int destIndex = y * stride + x * 4;

			// 复制颜色数据
			pPixels[destIndex + 0] = colorData[pixelIndex + 0];  // B
			pPixels[destIndex + 1] = colorData[pixelIndex + 1];  // G
			pPixels[destIndex + 2] = colorData[pixelIndex + 2];  // R

			// 根据遮罩数据设置透明度
			int maskByteIndex = (y * maskWidthBytes) + (x / 8);
			int maskBit = 7 - (x % 8);

			bool isMasked = (maskData[maskByteIndex] & (1 << maskBit)) == 0;
			pPixels[destIndex + 3] = isMasked ? 255 : 0;  // 255 = 不透明，0 = 透明
		}
	}
	bmp->UnlockBits(&bmpData);
	return bmp;
}

int xiaochufuji::IconExtract::GetEncoderClsid(CLSID& pClsid, const WCHAR* format)
{
	if (!m_gdiPlus->ok)  return -1;
	UINT num = 0;
	UINT size = 0;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0) {
		return -1;
	}
	std::unique_ptr< Gdiplus::ImageCodecInfo, void(*)(void*)> pImageCodecInfo(
		(Gdiplus::ImageCodecInfo*)malloc(size),
		[](void* p) { free(p); }
	);

	if (!pImageCodecInfo) {
		return -1;
	}

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo.get());

	for (UINT j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo.get()[j].MimeType, format) == 0) {
			pClsid = pImageCodecInfo.get()[j].Clsid;
			return j;  // get the encoder
		}
	}
	return -1;
}

std::vector<BYTE> xiaochufuji::IconExtract::gdiBitmapConvertPngMemoryStream(const std::unique_ptr<Gdiplus::Bitmap>& bitmap)
{
	std::vector<BYTE> pngData;
	CComPtr<IStream> pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
	if (FAILED(hr)) return {};
	CLSID pngClsid;
	if (!GetEncoderClsid(pngClsid, L"image/png")) {
		return {};
	}

	// Save bitmap to memory as PNG
	if (bitmap->Save(pStream, &pngClsid, nullptr) != Gdiplus::Ok) {
		return {};
	}

	// Extract PNG data from the stream
	STATSTG stat;
	pStream->Stat(&stat, STATFLAG_DEFAULT);
	ULONG dataSize = stat.cbSize.LowPart;

	pngData.resize(dataSize);
	LARGE_INTEGER seekStart = {};
	pStream->Seek(seekStart, STREAM_SEEK_SET, nullptr);
	ULONG readBytes;
	pStream->Read(pngData.data(), dataSize, &readBytes);
	return pngData;
}

UINT xiaochufuji::IconExtract::writeIconDir(std::ofstream& icoFile, int nImage)
{
	IconDir iconDir{};
	iconDir.idReserved = 0;
	iconDir.idType = 1; 
	iconDir.idCount = nImage;
	icoFile.write(reinterpret_cast<const char*>(&iconDir), sizeof(iconDir));
	if (!icoFile.good())  return 0;
	return sizeof(iconDir);
}

UINT xiaochufuji::IconExtract::writeIconDirEntry(std::ofstream& icoFile, BYTE bmpWidth, BYTE bmpHeight, DWORD pngSize, int index)
{
	IconDirEntry iconEntry{};
	iconEntry.bWidth = bmpWidth;
	iconEntry.bHeight = bmpHeight;
	iconEntry.bColorCount = 0;
	iconEntry.bReserved = 0;
	iconEntry.wPlanes = 1;
	iconEntry.wBitCount = 32;
	iconEntry.dwBytesInRes = static_cast<DWORD>(pngSize);
	iconEntry.dwImageOffset = sizeof(IconDir) + sizeof(IconDirEntry);
	icoFile.write(reinterpret_cast<const char*>(&iconEntry), sizeof(iconEntry));
	if (!icoFile.good())  return 0;
	return sizeof(iconEntry);
}

UINT xiaochufuji::IconExtract::writeIconData(std::ofstream& icoFile, const std::vector<BYTE>& pngData)
{
	icoFile.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
	if (!icoFile.good())  return 0;
	return sizeof(pngData.size());
}

bool xiaochufuji::IconExtract::saveIconPng(HICON hIcon, const _string& dstFile, int nNumIcons)
{
	// HICON conver to Gdiplus::Bitmap object
	auto bitmap = hiconConvertGdiBitmap(hIcon);
	if (!bitmap.get()) return false;

	// get png memory stream data by Gdiplus::Bitmap
	std::vector<BYTE> pngData = gdiBitmapConvertPngMemoryStream(bitmap);
	if (!pngData.size()) return false;

	// open a file and write icon file
	std::ofstream icoFile(dstFile, std::ios::binary);
	if (!icoFile.is_open()) return false;

	// write step
	if (writeIconDir(icoFile, 1) == 0) return false;
	if (writeIconDirEntry(icoFile, bitmap->GetWidth(), bitmap->GetHeight(), static_cast<DWORD>(pngData.size()), 0) == 0) return false;
	if (writeIconData(icoFile, pngData) == 0) return false;
	icoFile.close();
	return true;
}
