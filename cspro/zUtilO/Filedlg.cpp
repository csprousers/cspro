#include "StdAfx.h"
#include "Filedlg.h"
#include "FileUtil.h"


CIMSAFileDialog::CIMSAFileDialog(BOOL bOpenFileDialog,
                                 LPCTSTR lpszDefExt,
                                 LPCTSTR lpszFileName,
                                 DWORD dwFlags,
                                 LPCTSTR lpszFilter,
                                 CWnd* pParentWnd,
                                 UINT uDirType,
                                 BOOL bNewFile)
    :   CFileDialog(bOpenFileDialog, nullptr, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
    m_ofn.lpstrFileTitle = m_pszFileTitle;
    m_ofn.nMaxFileTitle = _MAX_PATH;

    m_bOpenFileDialog = bOpenFileDialog;
    m_bNewFile = bNewFile;
    m_uDirType = uDirType;

    if (uDirType == CFD_PROJ_DIR) {
        IMSAGetProjectDir();
    }
    else if (uDirType == CFD_DATA_DIR) {
        IMSAGetDataDir();
    }

    if (lpszDefExt == NULL) {
        _tcscpy(m_pszDefExt,_T(""));
    }
    else {
        _tcscpy(m_pszDefExt, lpszDefExt);
        TCHAR pszDefExt[64];
        TCHAR* pszExt;
        _tcscpy(pszDefExt, lpszDefExt);
        pszExt = _tcstok(pszDefExt,_T(", "));
        while (pszExt != NULL) {
            m_aDefExt.Add(pszExt);
            pszExt = _tcstok(NULL,_T(", "));
        }
    }
}


CIMSAFileDialog& CIMSAFileDialog::UseInitialDirectoryOfActiveDocument(CMDIFrameWnd* const pMDIFrameWnd)
{
    ASSERT(m_initialDirectory == nullptr);

    if( pMDIFrameWnd != nullptr )
    {
        CMDIChildWnd* const pActiveWnd = pMDIFrameWnd->MDIGetActive();

        if( pActiveWnd != nullptr )
        {
            const CDocument* const pDoc = pActiveWnd->GetActiveDocument();

            if( pDoc != nullptr )
            {
                m_initialDirectory = std::make_unique<std::wstring>(PortableFunctions::PathGetDirectory(pDoc->GetPathName()));

                if( !m_initialDirectory->empty() )
                    m_ofn.lpstrInitialDir = m_initialDirectory->c_str();
            }
        }
    }

    return *this;
}


CIMSAFileDialog& CIMSAFileDialog::SetMultiSelectBuffer(const size_t MaxFiles/* = 250*/)
{
    const size_t buffer_size = MaxFiles * ( _MAX_PATH + 1 ) + 1;
    m_multiSelectBuffer = std::make_unique<TCHAR[]>(buffer_size);

    m_ofn.lpstrFile = m_multiSelectBuffer.get();
    m_ofn.nMaxFile = buffer_size;

    return *this;
}


BOOL CIMSAFileDialog::OnFileNameOK()
{
    BOOL bReturn = FALSE;
    CString csFullFileName, csDir, csFileExt, csMessage, csTemp;

    //  Get file names
    m_aFileName.RemoveAll();

    POSITION p = GetStartPosition();
    while (p != NULL) {
        CString cs = GetNextPathName(p);
        int x = cs.Find(_T(':'), 2);
        if (x != NONE) {
            cs = cs.Mid(x - 1);
        }
        m_aFileName.Add(cs);
    }
//  Check file names
    for (int iIndex = 0 ; iIndex < m_aFileName.GetSize() ; iIndex++) {
        if (m_bOpenFileDialog && m_aFileName.GetSize() > 100) {
            break;
        }
        csFullFileName = m_aFileName[iIndex];
//  Check the file extension
        int nBackSlash = csFullFileName.ReverseFind(PATH_CHAR);
        int nFileExtension = csFullFileName.ReverseFind('.');
        if (nFileExtension == -1) {
            nFileExtension = csFullFileName.GetLength();
        }
        else if (nBackSlash > nFileExtension) {
            nFileExtension = csFullFileName.GetLength();
        }
        else {
            nFileExtension++;
        }
        csFileExt = csFullFileName.Mid(nFileExtension);
        // if needs an extension and a default extension is present
        if (csFileExt.IsEmpty()) {
            if (_tcslen(m_pszDefExt) > 0) {
                if (csFullFileName[csFullFileName.GetLength() - 1] != '.') {
                    // find extension in filter list
                    int iTimes = m_ofn.nFilterIndex * 2;
                    TCHAR* pszExt =(LPTSTR) m_ofn.lpstrFilter;
                    for (int i = 1 ; i < iTimes ; i++) {
                        pszExt = (LPTSTR)_tmemchr(pszExt, _T('\0'), 255);
                        pszExt++;
                    }
                    int iLen = _tcslen(pszExt);
                    if (_tmemchr(pszExt, _T(';'), iLen) == NULL) {
                        pszExt = (LPTSTR)_tmemchr(pszExt, _T('.'), iLen);
                        if (*(pszExt + 1) != '*') {
                            csFileExt = pszExt;
                            csFileExt = csFileExt.Mid(1);
                            csFullFileName += pszExt;
                            m_aFileName[iIndex] = csFullFileName;
                        }
                    }
                }
            }
        }
        if (m_aDefExt.GetSize() > 0) {
            BOOL bFound = FALSE;
            for (int i = 0 ; i < m_aDefExt.GetSize() ; i++) {
                if (csFileExt.CompareNoCase(m_aDefExt[i]) == 0) {
                    bFound = TRUE;
                    break;
                }
                if (m_aDefExt[i].CompareNoCase(_T("*")) == 0) {
                    bFound = TRUE;
                    break;
                }
            }
            if (!bFound) {
                if (m_aDefExt.GetSize() > 1) {
                    CString csLoad;
                    csLoad.LoadString(IDS_ONEEXTENSION);
                    csMessage.Format(csLoad, m_pszDefExt);
                    csMessage = csFullFileName + csMessage;
                }
                else {
                    CString csLoad;
                    csLoad.LoadString(IDS_THEEXTENSION);
                    csMessage.Format(csLoad, m_pszDefExt);
                    csMessage = csFullFileName + csMessage;
                }
                AfxMessageBox(csMessage,MB_OK | MB_ICONEXCLAMATION);
                bReturn = TRUE;
                continue;
            }
        }

//  Check the file existence

        CFileStatus status;
        BYTE mask = CFile::hidden | CFile::system | CFile::volume | CFile::directory;
        if (m_bOpenFileDialog) {
            if (m_bNewFile) {
                if (CFile::GetStatus(csFullFileName, status)) {
                    CString csLoad;
                    csLoad.LoadString(IDS_ALREADYEXISTS);
                    csMessage = csFullFileName + csLoad;
                    AfxMessageBox(csMessage,MB_OK | MB_ICONEXCLAMATION);
                    bReturn = TRUE;
                    continue;
                }
                CheckNewFile(csFullFileName, bReturn);
            }
            else {
                if (!CFile::GetStatus(csFullFileName, status)) {
                    CString csLoad;
                    csLoad.FormatMessage(_T("\n\n%1  file not found."), (LPCTSTR)csFullFileName);
                    csMessage = csLoad;
                    AfxMessageBox(csMessage,MB_OK | MB_ICONEXCLAMATION);
                    bReturn = TRUE;
                    continue;
                }
                if ((status.m_attribute & mask) != 0) {
                    CString csLoad;
                    csLoad.LoadString(IDS_NOTAFILE);
                    csMessage = csFullFileName + csLoad;
                    AfxMessageBox(csMessage,MB_OK | MB_ICONEXCLAMATION);
                    bReturn = TRUE;
                    continue;
                }
            }
        }
        else {
            if (CFile::GetStatus(csFullFileName, status)) {
                if ((status.m_attribute & mask) != 0) {
                    CString csLoad;
                    csLoad.LoadString(IDS_NOTAFILE);
                    csMessage = csFullFileName + csLoad;
                    AfxMessageBox(csMessage,MB_OK | MB_ICONEXCLAMATION);
                    bReturn = TRUE;
                    continue;
                }
            }
            else {
                CheckNewFile(csFullFileName, bReturn);
            }
        }
    }

//  Save file directory as IMSA Project Directory

    if (bReturn) {
        return bReturn;
    }

    TCHAR* pszFullFileName = csFullFileName.GetBuffer(_MAX_PATH);
    TCHAR* pszName;

    GetFullPathName(m_aFileName[0], _MAX_PATH, pszFullFileName, &pszName);
    *(pszName - 1) = _T('\0');

    csFullFileName.ReleaseBuffer();

    if (m_uDirType == CFD_PROJ_DIR) {
        IMSASetProjectDir(csFullFileName);
    }
    else if (m_uDirType == CFD_DATA_DIR) {
        IMSASetDataDir(csFullFileName);
    }

    _tcscpy(m_ofn.lpstrFile, m_aFileName[0]);

    int i = m_aFileName[0].ReverseFind(PATH_CHAR);

    _tcscpy(m_ofn.lpstrFileTitle, m_aFileName[0].Mid(i+1));

    return bReturn;
}


void CIMSAFileDialog::CheckNewFile(const CString& csFullFileName, BOOL& bReturn)
{
    CFile f;
    CFileException e;
    if(!f.Open(csFullFileName, CFile::modeCreate | CFile::modeWrite, &e)) {
        if (e.m_cause == CFileException::badPath) {
            CString csLoad;
            csLoad.LoadString(IDS_BADPATH);
            ErrorMessage::Display(csFullFileName + csLoad);
        }
        else {
            CString csLoad;
            csLoad.LoadString(IDS_CANTOPEN);
            ErrorMessage::Display(csFullFileName + csLoad);
        }
        bReturn = TRUE;
        return;
    }
    f.Close();
    f.Remove(csFullFileName);
    bReturn = FALSE;
}
