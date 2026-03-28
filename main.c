#include <windows.h>
#include <shellapi.h>
#include "resource.h"
#include "GetWndIco.h"

#define ID_HOTKEY_GRAB 0xC001

HINSTANCE hInst;

LRESULT MainDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

static char g_szLastTitle[256] = "default_icon";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    // Apply system DPI settings
	SetProcessDPIAware();
	
	const char* szUniqueName = "Local\\GrabWndIco_Unique_Mutex_v1.0";
	HANDLE hMutex = CreateMutexA(NULL, TRUE, szUniqueName);
	
	if (hMutex == NULL) {
		// System error, could not create mutex
		return 1;
	}
	
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Another instance is already running
		MessageBox(NULL,"Another instance is already running","Error",MB_ICONERROR);
		CloseHandle(hMutex);
		// Optional: You could find the existing window and BringWindowToTop here
		return 0; 
	}
	
	MSG msg;
	HWND hMainDlg = NULL;

	hInst = hInstance;
	hMainDlg = CreateDialog(hInstance, (LPCTSTR)IDD_MAIN_DIALOG, 0,(DLGPROC)MainDlgProc);
	
	if (!RegisterHotKey(hMainDlg, ID_HOTKEY_GRAB, MOD_CONTROL | MOD_SHIFT, 'G')) {
		MessageBox(hMainDlg, "Failed to register hotkey", "Error", MB_ICONERROR);
		ExitProcess(1);
	}
	
	ShowWindow(hMainDlg, nCmdShow);
	
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
		
	case WM_COMMAND:
//		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
//		{
//			EndDialog(hDlg, LOWORD(wParam));
//			return (INT_PTR)TRUE;
//		}
		switch(LOWORD(wParam)){
		case IDC_BTN_BLOG:
			ShellExecute(hDlg,"open","https://www.leapheap.net",NULL,NULL,SW_SHOWNORMAL);
			break;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG :
			return TRUE ;
		case WM_COMMAND :
			switch (LOWORD (wParam)) {
//				case IDOK :
//				case IDCANCEL :
//					DestroyWindow(hDlg);
//					return TRUE ;
			case IDC_BTN_ABOUT:
				DialogBox(GetModuleHandle(NULL), 
						  MAKEINTRESOURCE(IDD_ABOUT), 
						  hDlg, 
						  AboutDlgProc);
				break;
			case IDC_BTN_HELP:
				MessageBox(hDlg,"Press Ctrl + Shift + G to grab foreground window icon","Help",MB_ICONINFORMATION);
				break;
			case IDC_BTN_SAVE:
				HICON hIconToSave = (HICON)SendDlgItemMessage(hDlg, IDC_STC_ICOPREVIEW, STM_GETIMAGE, IMAGE_ICON, 0);
				if (hIconToSave){
					char szFinalFileName[300];
					wsprintf(szFinalFileName, "%s.ico", g_szLastTitle);
					if (GetSaveIconPath(hDlg,szFinalFileName)){
						SaveIconToFile(hIconToSave, szFinalFileName);
					}
				} else {
					MessageBox(hDlg,"Please grab icon first","Warning",MB_ICONWARNING);
				}
				break;
			}
			break ;
		case WM_HOTKEY:
			if (wParam == ID_HOTKEY_GRAB) {
				//GrabIcon(hDlg);
				HWND hPreviewStc = GetDlgItem(hDlg, IDC_STC_ICOPREVIEW);
				if (hPreviewStc){
					HICON hGrabbedIcon = NULL;
					HWND hForeground = GetForegroundWindow();
					if (hForeground) {
						// Set and sanitize default file name
						GetWindowText(hForeground, g_szLastTitle, sizeof(g_szLastTitle));
						SanitizeFileName(g_szLastTitle);
						
						// Display grabbed window handle
						char szHwnd[32];
						wsprintf(szHwnd, "0x%08X", (UINT_PTR)hForeground); 
						SetDlgItemText(hDlg, IDC_EDT_HWND, szHwnd);
						// Display grabbed window title
						char szTitle[256];
						GetWindowText(hForeground, szTitle, sizeof(szTitle));
						SetDlgItemText(hDlg,IDC_EDT_WTITLE,szTitle);
						// Grab window icon
						hGrabbedIcon = GetTargetWindowIcon(hForeground);
						
						if (hGrabbedIcon == NULL){
							hGrabbedIcon = CopyIcon(LoadIcon(NULL, IDI_APPLICATION));
						}
						// Display icon and clean old icon
						HICON hOld = (HICON)SendDlgItemMessage(hDlg, IDC_STC_ICOPREVIEW, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hGrabbedIcon);
						if (hOld) DestroyIcon(hOld); 
					}
					
				}
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hDlg);
			return TRUE;
		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
	};
	return FALSE;
}


