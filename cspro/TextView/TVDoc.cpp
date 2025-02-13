//***************************************************************************
//  File name: TVDoc.cpp
//
//  Description:
//       Implementation for TextView document
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include "StdAfx.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/UTF8Convert.h>
#include <zUtilO/Filedlg.h>
#include <fstream>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR pszSaveAsLastUsed[] = _T("Last used") _T("\0") _T("*.*") _T("\0") _T("\0") _T("                                                         ");

/////////////////////////////////////////////////////////////////////////////
// CTVDoc

IMPLEMENT_DYNCREATE(CTVDoc, CDocument)

BEGIN_MESSAGE_MAP(CTVDoc, CDocument)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                             CTVDoc::CTVDoc
//
/////////////////////////////////////////////////////////////////////////////

CTVDoc::CTVDoc()
    :   m_bIsReloadingOrClosing(false) // = true if we're in the middle of a reload/close deleted file operation
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTVDoc::OnOpenDocument
//
/////////////////////////////////////////////////////////////////////////////

BOOL CTVDoc::OnOpenDocument(const TCHAR* lpszPathName) {

    CTVDoc* pDoubleDoc = ((CTextViewApp*)AfxGetApp())->FindFile(lpszPathName);
    if (pDoubleDoc && !pDoubleDoc->IsReloadingOrClosing())  {
        ASSERT_VALID(pDoubleDoc);
        TRACE(_T("Closing duplicate document %p, %s %s\n"), pDoubleDoc, (const TCHAR*) pDoubleDoc->GetTitle(), (const TCHAR*) pDoubleDoc->GetPathName());
        pDoubleDoc->OnCloseDocument();
    }

    if (!m_buffMgr.GetFileIO()->SetFileName(lpszPathName)) {
        return FALSE;
    }

    if ( m_buffMgr.GetFileIO()->Open() )  {
        ((CTextViewApp*) AfxGetApp())->AddToRecentFileList(lpszPathName);
        SetTitle (m_buffMgr.GetFileIO()->GetFileName());
        SetPathName(GetTitle());
        UpdateStatusBar();
        AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last File"),lpszPathName);
    }
    else  {
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTVDoc::OnFileSaveAs
//
/////////////////////////////////////////////////////////////////////////////

void CTVDoc::OnFileSaveAs() {

    CString csFileName;
    POSITION pos = GetFirstViewPosition();     // the first view position is the "main" view -- the one created first!
    CBlockScrollView* pView = (CBlockScrollView*) GetNextView(pos);

    //  If selection, do you want all or selection.
    BOOL bAll = TRUE;
    BOOL bSelected = pView->IsBlockActive();
    if (bSelected) {
        CSelectionDialog seldlg;
        seldlg.SetView (pView);              // let it know who it's owner is (CGotoDialog::OnSize will reposition the dialog box in a moment)
        seldlg.m_nAll = 1;
        if (seldlg.DoModal() == IDCANCEL) {
            return;
        }
        if (seldlg.m_nAll == 1) {
            bAll = FALSE;
        }
    }

    //  Get file name
    CString csFilter;
    csFilter.LoadString(IDS_MSG03);
    CIMSAFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter );
    dlg.m_ofn.lpstrCustomFilter = pszSaveAsLastUsed;
    dlg.m_ofn.nMaxCustFilter = MAXFILTER;
    BOOL bOK = FALSE;
    while (!bOK) {
        // until we get a good file name or the user cancels the dialog ...
        if (dlg.DoModal() == IDCANCEL) {
            return;
        }
        // can't save back to the same file     gsf 09/22/97
        if (dlg.GetPathName() == GetPathName()) {
            AfxMessageBox(_T("Can't replace viewed file.  Use a different name."));
            continue;
        }
        csFileName = dlg.GetPathName();
        bOK = TRUE;
    }

    //  Do save as
    CBufferMgr* pBuffMgr = GetBufferMgr();
    CString csLine, csPadding = _T("        ");
    long lPrevLine = pBuffMgr->GetCurrLine();
    long j;
    int nOffset;

    if (bAll){
        // Whole file
        //Just copy the entire file with BOM
        //pFile->close();
        if(::CopyFile(GetPathName(),csFileName,false) == FALSE){
            CString csTmp;
            csTmp.LoadString (IDS_ERR07);
            csTmp += csFileName;
            AfxMessageBox(csTmp, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }
    else {
        try {
            std::unique_ptr<std::ofstream> os = FileIO::OpenOutputFileStream(csFileName);
            os->write(Utf8BOM_sv.data(), Utf8BOM_sv.length());

            // Selected text
            CLRect rclBlockedRect = pView->GetBlockedRectChar();
            ASSERT(rclBlockedRect.top >= 0);
            ASSERT(rclBlockedRect.left >= 0);
            int iLen = (int) (rclBlockedRect.right - rclBlockedRect.left + 1);
            int iLeft = (int) rclBlockedRect.left;
            pBuffMgr->GotoLineNumber(rclBlockedRect.top);
            for (j = rclBlockedRect.top; j <= rclBlockedRect.bottom; j++) {
                csLine = pBuffMgr->GetNextLine();
                while ( (nOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                    csLine = csLine.Left (nOffset) + csPadding.Left (TAB_SPACES-(nOffset % TAB_SPACES)) + csLine.Mid (nOffset+1);
                }
                std::string utf_text = UTF8Convert::WideToUTF8(wstring_view(csLine).substr(iLeft, iLen));
                os->write(utf_text.data(), utf_text.length());
                os->write("\r\n", 2);
            }
            os->close();
        } catch( const CSProException& exception) {
            ErrorMessage::Display(exception);
            return;
        }
    }
    pBuffMgr->GotoLineNumber (lPrevLine);

    AfxGetApp()->AddToRecentFileList(csFileName);

}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTVDoc::UpdateStatusBar
//
/////////////////////////////////////////////////////////////////////////////

void CTVDoc::UpdateStatusBar()  {

    long lSize = m_buffMgr.GetFileSize();
    TCHAR caFileSize[6];
    TCHAR* caTmp;
    CString csStr, csSizeMsg;

    // replace gobs of zeros with descriptions like MB, KB ... if applicable
    if ( lSize >= 10000000L )  {
        lSize /= 1000000L;
        csStr = _T(" MB");
    }
    else  {
        if ( lSize >= 10000L )  {
            lSize /= 1000L;
            csStr = _T(" KB");
        }
        else  {
            csStr.LoadString (IDS_MSG04);      // = " bytes";
        }
    }

    caFileSize[5] = EOS;
    ltoc (caFileSize, lSize, 5, ' ');
    caTmp = (TCHAR*) caFileSize;
    while ( *caTmp == ' ')  {
        // traverse the string to get past the padding spaces ...
        caTmp++;
    }

    csSizeMsg.LoadString (IDS_MSG05);   // "Size"
    csStr = caTmp + csStr;
    csStr = csSizeMsg + csStr;

    ((CMainFrame*) AfxGetApp()->m_pMainWnd)->UpdateStatusBarSize ( (const TCHAR*) csStr);

    switch( m_buffMgr.GetFileEncoding() ) // GHM 20111222
    {
    case Encoding::Utf8:
        caTmp = _T("UTF-8");
        break;
    case Encoding::Utf16LE:
        caTmp = _T("UTF-16LE");
        break;
    default:
        caTmp = _T("ANSI");
    }

    csStr.Format(_T("Encoding: %s"),caTmp);

    ((CMainFrame*) AfxGetApp()->m_pMainWnd)->UpdateStatusBarEncoding((const TCHAR*)csStr);
}

void CTVDoc::ReloadFile()
{
    CString cs, csFile;

/*
    CMDIChildWnd* pChild = MDIGetActive();
    ASSERT(pChild != NULL);
    ASSERT_KINDOF(CTVDoc, pChild->GetActiveDocument());
    CTVDoc* pDoc = (CTVDoc*) pChild->GetActiveDocument();
    csFile = pDoc->GetPathName();

    SendMessage(WM_COMMAND, ID_FILE_CLOSE);
    AfxGetApp()->OpenDocumentFile(csFile);
*/

    if (!IsReloadingOrClosing()) {    // prevent recursion
        m_bIsReloadingOrClosing = true;
        csFile = GetPathName();

        // GHM 20101212 TextView crashed if passed a directory name; this fixes that
        if( csFile.IsEmpty() )
            return;

        cs.Format(_T("File %s has been changed by another application and will be reloaded."), (LPCTSTR)csFile);
        AfxMessageBox(cs, MB_ICONEXCLAMATION);

        AfxGetApp()->OpenDocumentFile(csFile); //opendocument file is overridden to open a copy when reloading or closing
        OnCloseDocument(); //finally close the old copy
    }
}


void CTVDoc::CloseDeletedFile()
{
    if (!IsReloadingOrClosing()) {    // prevent recursion
        m_bIsReloadingOrClosing = true;
        AfxMessageBox(FormatText(_T("File %s has been deleted, or is no longer available. File will be closed."), (LPCTSTR)GetPathName()), MB_ICONEXCLAMATION);
        OnCloseDocument();
    }
}
