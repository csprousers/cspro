#include "StdAfx.h"
#include "DatFDlg.h"
#include <zUtilO/Interapp.h>
#include <zMessageO/Messages.h>


CDatFDlg::CDatFDlg(bool allowCSProExtensions, BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                   DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
    :   CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd),
        m_bAllowCSProExtensions(allowCSProExtensions)
{
}


BOOL CDatFDlg::OnFileNameOK()
{
    // 20130410 to disallow users from using CSPro file extensions for their data files
    const std::string extension = TC::ToUtf8(GetFileExt());

    if( !m_bAllowCSProExtensions && FileExtensions::IsExtensionForbiddenForDataFiles(extension) )
    {
        AfxMessageBox(FormatText("CSPro data files cannot have the file extension: .%s", extension.c_str()));
        return TRUE;
    }

    if( !PortableFunctions::FileIsRegular(GetPathName()) ) // 20120212
    {
        const std::string message = FormatText(MGF::GetMessageText(MGF::CreateNewFile)->c_str(), UTF8_TODO::GetUtf8(GetFileName()).c_str());

        if( AfxMessageBox(message, MB_YESNO | MB_ICONEXCLAMATION) == IDNO )
            return TRUE;
    }

    return CFileDialog::OnFileNameOK();
}
