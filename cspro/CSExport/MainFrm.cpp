#include "StdAfx.h"
#include "MainFrm.h"
#include "CSExport.h"
#include "ExptDoc.h"
#include "ExptView.h"
#include <zUtilO/Interapp.h>
#include <zEditO/Lexers.h>
#include <zInterfaceF/UWM.h>


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_MENUCHAR()
    ON_UPDATE_COMMAND_UI(ID_FILE_RUN, OnUpdateFileRun)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
    ON_MESSAGE(WM_IMSA_EXPORTDONE, OnIMSAExportDone)
    ON_MESSAGE(UWM::Interface::SelectLanguage, OnSelectLanguage)
    ON_MESSAGE(UWM::Edit::GetLexerLanguage, OnGetLexerLanguage)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    //Create language bar
    if (!m_wndLangDlgBar.Create(this, IDD_LANGDLGBAR, CBRS_ALIGN_TOP, IDD_LANGDLGBAR))
    {
        TRACE0("Failed to create language dialogbar \n");
        return -1;      // fail to create
    }

    // Create rebar
    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar) ||
        !m_wndReBar.AddBar(&m_wndLangDlgBar))
    {
        /*
            These ints are positions of the bars defined at the top of this file. I
            While adding a bar make sure that the positions are specified corrrectly.
            const int TOOLBARPOS = 0;
            const int LANGTOOLBARPOS = 1;
        */
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }

    //  Places band 1 (Language Bar) flush next to toolbar.
    m_wndReBar.GetReBarCtrl().RestoreBand(1);

    //  default the language bar off.
    m_wndReBar.GetReBarCtrl().ShowBand(1, FALSE);

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
  LRESULT lresult;
  if(BCMenu::IsMenu(pMenu))
    lresult=BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
    else
    lresult=CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
  return(lresult);
}



bool CMainFrame::PostRunFileCheck(CString csFilename)
{
    if( !csFilename.IsEmpty() )
    {
        if( !PortableFunctions::FileExists(csFilename) )
        {
            CString csRunErrorFilename;
            csRunErrorFilename.Format(_T("%sCSExpRun%d.err"), UTF8_TODO::GetWide(GetTempDirectory()).c_str(), GetCurrentProcessId());
            bool bHadCompilationWarnings = PortableFunctions::FileExists(csRunErrorFilename);

            CIMSAString sMsg;
            sMsg.Format(_T("No output generated in file %s. File not created."), csFilename.GetString());

            if( bHadCompilationWarnings )
                sMsg.Append(_T(" There were compilation warnings so the universe may not be valid."));

            AfxMessageBox(sMsg);
        }

        else
            ViewFileInTextViewer(csFilename);
    }

    return true;
}


LRESULT CMainFrame::OnIMSAExportDone(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CExportDoc* pDoc = (CExportDoc*)GetActiveDocument();
    CExportApp* pApp = (CExportApp*) AfxGetApp();
    if(!pApp){
        if(pDoc->m_batchmode) {
            PostMessage(WM_CLOSE);
        }
        return 0;
    }

    if(!pDoc){

        POSITION pos = pApp->GetFirstDocTemplatePosition();
        CSingleDocTemplate* pTemplate = (CSingleDocTemplate*)pApp->GetNextDocTemplate(pos);
        ASSERT(pTemplate);
        pos = pTemplate->GetFirstDocPosition();
        if(pos != NULL){
            pDoc = (CExportDoc *)pTemplate->GetNextDoc(pos);
        }
    }

    ASSERT(pDoc);
    pDoc->DoPostRunSave();
    if (!pDoc->m_PifFile.GetViewResultsFlag()) {    // BMD 31 Aug 2006
        if(pDoc->m_batchmode) {
            PostMessage(WM_CLOSE);
        }
        return 0;
    }
    if( !pDoc->m_csSPSSOutFile.IsEmpty() )
    {
        if( !PostRunFileCheck(pDoc->m_csSPSSOutFile) ||
            !PostRunFileCheck(pDoc->m_csCSProDCFFile) ||
            !PostRunFileCheck(pDoc->m_csSPSSDescFile) ||
            !PostRunFileCheck(pDoc->m_csSASDescFile) ||
            !PostRunFileCheck(pDoc->m_csSTATADescFile) ||
            !PostRunFileCheck(pDoc->m_csSTATALabelFile) ||
            !PostRunFileCheck(pDoc->m_csRDescFile) )
        {
            if( pDoc->m_batchmode )
                PostMessage(WM_CLOSE);

            return 0;
        }
    }
    else {
        pDoc->GetNumExpRecTypes();
        CFileStatus fStatus;
        for (int i=0;  i < ((CExportApp *)AfxGetApp())->m_arrPifInfo.GetSize(); i++) {
            if(i >= ((CExportApp *)AfxGetApp())->m_arrPifInfo.GetSize()){
                break;
            }

            CIMSAString csDataFile = ((CExportApp *)AfxGetApp())->m_arrPifInfo[i]->sFileName;

            if( !PostRunFileCheck(csDataFile) )
            {
                if( pDoc->m_batchmode )
                    PostMessage(WM_CLOSE);

                return 0;
            }

            switch (pDoc->m_convmethod)
            {
                case METHOD::SPSS:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::SpssSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN,csDataFile);
                }
                break;

                case METHOD::SAS:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::SasSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);
                }
                break;

                case METHOD::STATA:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::StataDictionary);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::StataDo);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);
                }
                break;

                case METHOD::R:
                {
                    // 20140520 I forgot to add this, for an export to multiple files / record, when I initially implemented the R export
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::RSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);
                }
                break;

                case METHOD::CSPRO:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);
                }
                break;

                case METHOD::ALLTYPES:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::SpssSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN,csDataFile);

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::SasSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::StataDictionary);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::StataDo);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH)); // 20140520
                    csDataFile.ReleaseBuffer();
                    csDataFile += _T(".") + UTF8_TODO::GetCString(FileExtensions::RSyntax);
                    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, csDataFile);
                }
                break;
            }
        }
        ((CExportApp *)AfxGetApp())->DeletePifInfos();
    }
    if(pDoc->m_batchmode) {
        PostMessage(WM_CLOSE);
    }
    return 0;
}

void CMainFrame::OnUpdateFileRun(CCmdUI* pCmdUI)
{
    CExportApp* pApp = (CExportApp*) AfxGetApp();
    if(!pApp)
        return;
    POSITION pos = pApp->GetFirstDocTemplatePosition();
    CSingleDocTemplate* pTemplate = (CSingleDocTemplate*)pApp->GetNextDocTemplate(pos);
    ASSERT(pTemplate);
    pos = pTemplate->GetFirstDocPosition();
    if(pos != NULL){

        CExportDoc* pDoc = (CExportDoc *)pTemplate->GetNextDoc(pos);
        if(pDoc && pDoc->GetNumItemsSelected() > 0){
            pCmdUI->Enable(TRUE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnFileRun()
{
    CExportApp* pApp = (CExportApp*) AfxGetApp();
    if(!pApp)
        return;
    POSITION pos = pApp->GetFirstDocTemplatePosition();
    CSingleDocTemplate* pTemplate = (CSingleDocTemplate*)pApp->GetNextDocTemplate(pos);
    ASSERT(pTemplate);
    pos = pTemplate->GetFirstDocPosition();
    if(pos != NULL){

        CExportDoc* pDoc = (CExportDoc *)pTemplate->GetNextDoc(pos);

        if(pDoc && pDoc->GetNumItemsSelected() > 0){
            pDoc->ProcessRun();
        }
    }
}

void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/)
{
    CString csAppName;
    csAppName.Format(AFX_IDS_APP_TITLE);

    CExportDoc* pDoc = (CExportDoc*)GetActiveDocument();

    std::string title = ( pDoc != nullptr ) ? pDoc->GetDocumentWindowTitle() : std::string();
    SO::AppendWithSeparator(title, UTF8_TODO::GetUtf8(csAppName.GetString()), " - ");

    WindowsUtf8::SetText(this, title);
}


LRESULT CMainFrame::OnSelectLanguage(WPARAM wParam, LPARAM /*lParam*/)
{
    const CExportDoc* pDoc = assert_nullable_cast<const CExportDoc*>(GetActiveDocument());
    if (pDoc)
    {
        const CDataDict* pDataDict = pDoc->GetDataDict();
        if (pDataDict)
        {
            pDataDict->SetCurrentLanguage(wParam);
            // redraw the tree.
            CExportView* pExportView = reinterpret_cast<CExportView*>(GetActiveView());
            pExportView->RefreshView();
        }
    }

    return 1;
}


LRESULT CMainFrame::OnGetLexerLanguage(WPARAM /*wParam*/, LPARAM lParam)
{
    int& logic_language = *reinterpret_cast<int*>(lParam);

    const CExportDoc* pDoc = assert_cast<const CExportDoc*>(GetActiveDocument());
    logic_language = Lexers::GetLexer_Logic(pDoc->GetLogicSettings());

    return 1;
}
