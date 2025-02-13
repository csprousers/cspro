#include "StdAfx.h"
#include "DatFDlg.h"
#include <zUtilO/Interapp.h>
#include <zMessageO/Messages.h>


CDatFDlg::CDatFDlg(bool allowCSProExtensions, BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
        DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
        CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd),
        m_bAllowCSProExtensions(allowCSProExtensions)
{
}


BOOL CDatFDlg::OnFileNameOK()
{
    // 20130410 to disallow users from using CSPro file extensions for their data files
    if( !m_bAllowCSProExtensions && FileExtensions::IsExtensionForbiddenForDataFiles(GetFileExt()) )
    {
        AfxMessageBox(FormatText(_T("CSPro data files cannot have the file extension: .%s"), (LPCTSTR)GetFileExt()));
        return TRUE;
    }

    if( !PortableFunctions::FileIsRegular(GetPathName()) ) // 20120212
    {
        CString msg = FormatText(MGF::GetMessageText(MGF::CreateNewFile).c_str(), GetFileName().GetString());

        if( AfxMessageBox(msg, MB_YESNO | MB_ICONEXCLAMATION) == IDNO )
            return TRUE;
    }

    return CFileDialog::OnFileNameOK();
}
