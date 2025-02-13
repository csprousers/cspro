#include "StdAfx.h"
#include "CSPro.h"
#include "SymbolAnalysisDlg.h"
#include <zDesignerF/CodeMenu.h>


// --------------------------------------------------------------------------
// helpers
// --------------------------------------------------------------------------

namespace
{
    std::tuple<CDocument*, CLogicCtrl*> GetActiveDocumentAndLogicCtrl(CMainFrame* pMainFrame);
    std::tuple<const Application*, CLogicCtrl*> GetActiveApplicationAndLogicCtrl(CMainFrame* pMainFrame);
    CLogicCtrl* GetActiveLogicCtrl(CMainFrame* pMainFrame);
    bool IsLogicCtrlActive(CMainFrame* pMainFrame);


    std::tuple<CDocument*, CLogicCtrl*> GetActiveDocumentAndLogicCtrl(CMainFrame* pMainFrame)
    {
        ASSERT(pMainFrame != nullptr);

        CFrameWnd* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveFrame();
        ASSERT(pFrame != nullptr);

        CView* pView = pFrame->GetActiveView();

        CLogicCtrl* logic_ctrl = ( pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView)) ) ? assert_cast<CFSourceEditView*>(pView)->GetLogicCtrl() :
                                 ( pView->IsKindOf(RUNTIME_CLASS(COSourceEditView)) ) ? assert_cast<COSourceEditView*>(pView)->GetLogicCtrl() :
                                                                                        nullptr;

        return { pFrame->GetActiveDocument(), logic_ctrl };
    }


    std::tuple<const Application*, CLogicCtrl*> GetActiveApplicationAndLogicCtrl(CMainFrame* pMainFrame)
    {
        const Application* application = nullptr;
        CDocument* document;
        CLogicCtrl* logic_ctrl;
        std::tie(document, logic_ctrl) = GetActiveDocumentAndLogicCtrl(pMainFrame);

        if( document != nullptr )
        {
            CAplDoc* pAplDoc = pMainFrame->ProcessFOForSrcCode(*document);
            
            if( pAplDoc != nullptr )
                application = &pAplDoc->GetAppObject();
        }

        return { application, logic_ctrl };
    }


    CLogicCtrl* GetActiveLogicCtrl(CMainFrame* pMainFrame)
    {
        return std::get<1>(GetActiveDocumentAndLogicCtrl(pMainFrame));
    }


    bool IsLogicCtrlActive(CMainFrame* pMainFrame)
    {
        return ( GetActiveLogicCtrl(pMainFrame) != nullptr );
    }
}


void CMainFrame::OnUpdateIfLogicIsShowing(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsLogicCtrlActive(this));
}



// --------------------------------------------------------------------------
// Paste as String Literal
// --------------------------------------------------------------------------

void CMainFrame::OnPasteStringLiteral()
{
    CLogicCtrl* logic_ctrl = assert_cast<CLogicCtrl*>(GetFocus());
    CodeMenu::OnPasteStringLiteral(*logic_ctrl);
}


void CMainFrame::OnUpdatePasteStringLiteral(CCmdUI* pCmdUI)
{
    bool enable = false;

    if( WinClipboard::HasText() )
    {
        CWnd* focused_wnd = GetFocus();
        ASSERT(focused_wnd != nullptr);

        enable = ( focused_wnd->IsKindOf(RUNTIME_CLASS(CLogicCtrl)) &&
                   CodeMenu::CanPasteStringLiteral(assert_cast<CLogicCtrl*>(focused_wnd)->GetLexer()) );
    }

    pCmdUI->Enable(enable);
}



// --------------------------------------------------------------------------
// String Encoder
// --------------------------------------------------------------------------

void CMainFrame::OnStringEncoder()
{
    const Application* application;
    CLogicCtrl* logic_ctrl;
    std::tie(application, logic_ctrl) = GetActiveApplicationAndLogicCtrl(this);

    const LogicSettings* logic_settings = nullptr;
    std::wstring initial_text;

    if( application != nullptr )
    {
        logic_settings = &application->GetLogicSettings();

        if( logic_ctrl != nullptr )
            initial_text = logic_ctrl->GetSelText();
    }

    // if no logic settings are available, it means that a form file is open; in that case, use the user default logic settings
    CodeMenu::OnStringEncoder(( logic_settings != nullptr ) ? *logic_settings : LogicSettings::GetUserDefaultSettings(), std::move(initial_text));
}



// --------------------------------------------------------------------------
// Path Adjuster
// --------------------------------------------------------------------------

void CMainFrame::OnPathAdjuster()
{
    Application* application;
    int lexer_language;
    std::wstring initial_path;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) == 1 )
    {
        lexer_language = Lexers::GetLexer_Logic(*application);
        initial_path = CS2WS(application->GetApplicationFilename());
    }

    else
    {
        // if no application is available, it means that a form file is open; in that case, use the user default logic settings
        lexer_language = Lexers::GetLexer_Logic(LogicSettings::GetUserDefaultSettings());

        CDocument* document;
        std::tie(document, std::ignore) = GetActiveDocumentAndLogicCtrl(this);

        if( document != nullptr )
            initial_path = CS2WS(document->GetPathName());
    }

    CodeMenu::OnPathAdjuster(lexer_language, std::move(initial_path));
}



// --------------------------------------------------------------------------
// Symbol Analysis
// --------------------------------------------------------------------------

void CMainFrame::OnSymbolAnalysis()
{
    try
    {
        CMDIChildWnd* pWnd = MDIGetActive();
        CAplDoc* pAplDoc = ProcessFOForSrcCode(*pWnd->GetActiveDocument());

        if( pAplDoc == nullptr || pAplDoc->IsAppModified() )
            throw CSProException("Please save your application before running the symbol analyis.");

        Application& application = pAplDoc->GetAppObject();
        SymbolAnalysisCompiler symbol_analysis_compiler(application);
        symbol_analysis_compiler.Compile();

        if( symbol_analysis_compiler.GetSymbolUseMap().empty() )
            throw CSProException("The application does not have any symbols.");

        SymbolAnalysisDlg symbol_analysis_dlg(application, symbol_analysis_compiler);
        symbol_analysis_dlg.DoModal();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}



// --------------------------------------------------------------------------
// Deprecation Warnings
// --------------------------------------------------------------------------

void CMainFrame::OnDeprecationWarnings(UINT nID)
{
    CodeMenu::DeprecationWarnings::OnDeprecationWarnings(nID);
}


void CMainFrame::OnUpdateDeprecationWarnings(CCmdUI* pCmdUI)
{
    CodeMenu::DeprecationWarnings::OnUpdateDeprecationWarnings(pCmdUI);
}



// --------------------------------------------------------------------------
// Code Folding
// --------------------------------------------------------------------------

void CMainFrame::OnCodeFoldingLevel(UINT nID)
{
    CodeMenu::CodeFolding::OnCodeFoldingLevel(nID, GetActiveLogicCtrl(this));
}


void CMainFrame::OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI)
{
    CodeMenu::CodeFolding::OnUpdateCodeFoldingLevel(pCmdUI);
}


void CMainFrame::OnCodeFoldingAction(UINT nID)
{
    CodeMenu::CodeFolding::OnCodeFoldingAction(nID, GetActiveLogicCtrl(this));
}


void CMainFrame::OnUpdateCodeFoldingAction(CCmdUI* pCmdUI)
{
    CodeMenu::CodeFolding::OnUpdateCodeFoldingAction(pCmdUI, GetActiveLogicCtrl(this));
}
