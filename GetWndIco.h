#ifndef GETWNDICO_H
#define GETWNDICO_H

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

BOOL GetSaveIconPath(HWND hWnd, char* szOutPath);
HICON GetTargetWindowIcon(HWND hWnd);
BOOL SaveIconToFile(HICON hIcon, const char* szFileName);
void SanitizeFileName(char* pszName);

#endif
