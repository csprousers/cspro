#pragma once

// Executables should include this file, and call InitializeCommonControls in their
// CWinApp::InitInstance override, to enable visual styles.


// Use ComCtl32.dll version 6, done via a pragma rather than requiring a manifest file.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


inline void InitializeCommonControls(const DWORD additional_classes = 0)
{
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);

    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES | additional_classes;

    InitCommonControlsEx(&InitCtrls);
};
