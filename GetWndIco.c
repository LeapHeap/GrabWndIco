#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include "GetWndIco.h"


// Global variable for the filename dialog
char g_szFileName[MAX_PATH] = "icon.ico";

BOOL GetSaveIconPath(HWND hWnd, char* szOutPath) {
	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Icon Files (*.ico)\0*.ico\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szOutPath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = "ico";
	
	return GetSaveFileNameA(&ofn);
}

HICON GetTargetWindowIcon(HWND hWnd) {
	HICON hIcon = NULL;
	// 1. Try WM_GETICON (Big)
	if (!SendMessageTimeout(hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 500, (PDWORD_PTR)&hIcon) || !hIcon) {
		// 2. Try WM_GETICON (Small/Small2)
		SendMessageTimeout(hWnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 250, (PDWORD_PTR)&hIcon);
	}
	
	// 3. Try Window Class
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
	}
	return (hIcon) ? CopyIcon(hIcon) : NULL;
}

// Fixed SaveIconToFile with Vertical Flip and Win32 API
BOOL SaveIconToFile(HICON hIcon, const char* szFileName) {
	if (!hIcon) return FALSE;
	
	ICONINFO ii;
	if (!GetIconInfo(hIcon, &ii)) return FALSE;
	
	BITMAP bmpColor;
	GetObject(ii.hbmColor, sizeof(BITMAP), &bmpColor);
	
	HANDLE hFile = CreateFileA(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	
	DWORD bytesWritten;
	WORD header[3] = { 0, 1, 1 };
	WriteFile(hFile, header, sizeof(header), &bytesWritten, NULL);
	
	DWORD dwHeaderSize = sizeof(BITMAPINFOHEADER);
	DWORD dwBitsSize = bmpColor.bmWidthBytes * bmpColor.bmHeight;
	DWORD dwMaskSize = ((bmpColor.bmWidth + 31) / 32 * 4) * bmpColor.bmHeight;
	
	// ICONDIRENTRY
	BYTE entry[16] = {0};
	entry[0] = (BYTE)bmpColor.bmWidth;
	entry[1] = (BYTE)bmpColor.bmHeight;
	*(WORD*)&entry[4] = 1; 
	*(WORD*)&entry[6] = 32; 
	*(DWORD*)&entry[8] = dwHeaderSize + dwBitsSize + dwMaskSize;
	*(DWORD*)&entry[12] = 22; // 6 (Header) + 16 (Entry)
	WriteFile(hFile, entry, 16, &bytesWritten, NULL);
	
	// BITMAPINFOHEADER
	BITMAPINFOHEADER bih = {0};
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = bmpColor.bmWidth;
	bih.biHeight = bmpColor.bmHeight * 2; // ICO requires XOR + AND height
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	WriteFile(hFile, &bih, sizeof(bih), &bytesWritten, NULL);
	
	// Pixel Data with Vertical Flip
	BYTE* pPixels = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBitsSize);
	if (pPixels) {
		GetBitmapBits(ii.hbmColor, dwBitsSize, pPixels);
		
		// --- Manual Row Flip Logic ---
		BYTE* pRowTemp = (BYTE*)HeapAlloc(GetProcessHeap(), 0, bmpColor.bmWidthBytes);
		for (int y = 0; y < bmpColor.bmHeight / 2; y++) {
			BYTE* pTopRow = pPixels + (y * bmpColor.bmWidthBytes);
			BYTE* pBottomRow = pPixels + ((bmpColor.bmHeight - 1 - y) * bmpColor.bmWidthBytes);
			CopyMemory(pRowTemp, pTopRow, bmpColor.bmWidthBytes);
			CopyMemory(pTopRow, pBottomRow, bmpColor.bmWidthBytes);
			CopyMemory(pBottomRow, pRowTemp, bmpColor.bmWidthBytes);
		}
		HeapFree(GetProcessHeap(), 0, pRowTemp);
		// --- End of Flip ---
		
		WriteFile(hFile, pPixels, dwBitsSize, &bytesWritten, NULL);
		HeapFree(GetProcessHeap(), 0, pPixels);
	}
	
	// Mask Data (AND Mask) - Usually fine without flip as it's often blank or simple
	BYTE* pMask = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwMaskSize);
	if (pMask) {
		GetBitmapBits(ii.hbmMask, dwMaskSize, pMask);
		WriteFile(hFile, pMask, dwMaskSize, &bytesWritten, NULL);
		HeapFree(GetProcessHeap(), 0, pMask);
	}
	
	CloseHandle(hFile);
	DeleteObject(ii.hbmColor);
	DeleteObject(ii.hbmMask);
	return TRUE;
}

void SanitizeFileName(char* pszName) {
	// Define illegal characters in file name
	const char* illegalChars = "\\/:*?\"<>|";
	for (; *pszName; pszName++) {
		if (strchr(illegalChars, *pszName)) {
			*pszName = '_'; // Replace illegal characters with _
		}
	}
}
