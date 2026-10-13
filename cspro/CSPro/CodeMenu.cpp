#include "StdAfx.h"
#include "CSPro.h"
#include "SymbolAnalysisDlg.h"
#include <zDesignerF/CodeMenu.h>


// --------------------------------------------------------------------------
// helpers
// --------------------------------------------------------------------------

namespace
{
    std::tuple<CDocument*, LogicCtrl*> GetActiveDocumentAndLogicCtrl(CMainFrame* pMainFrame);
    std::tuple<const Application*, LogicCtrl*> GetActiveApplicationAndLogicCtrl(CMainFrame* pMainFrame);
    LogicCtrl* GetActiveLogicCtrl(CMainFrame* pMainFrame);
    bool IsLogicCtrlActive(CMainFrame* pMainFrame);


    std::tuple<CDocument*, LogicCtrl*> GetActiveDocumentAndLogicCtrl(CMainFrame* pMainFrame)
    {
        ASSERT(pMainFrame != nullptr);

        CFrameWnd* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveFrame();
        ASSERT(pFrame != nullptr);

        CView* pView = pFrame->GetActiveView();

        LogicCtrl* logic_ctrl = ( pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView)) ) ? assert_cast<CFSourceEditView*>(pView)->GetLogicCtrl() :
                                ( pView->IsKindOf(RUNTIME_CLASS(COSourceEditView)) ) ? assert_cast<COSourceEditView*>(pView)->GetLogicCtrl() :
                                                                                       nullptr;

        return { pFrame->GetActiveDocument(), logic_ctrl };
    }


    std::tuple<const Application*, LogicCtrl*> GetActiveApplicationAndLogicCtrl(CMainFrame* pMainFrame)
    {
        const Application* application = nullptr;
        CDocument* document;
        LogicCtrl* logic_ctrl;
        std::tie(document, logic_ctrl) = GetActiveDocumentAndLogicCtrl(pMainFrame);

        if( document != nullptr )
        {
            CAplDoc* pAplDoc = pMainFrame->ProcessFOForSrcCode(*document);

            if( pAplDoc != nullptr )
                application = &pAplDoc->GetAppObject();
        }

        return { application, logic_ctrl };
    }


    LogicCtrl* GetActiveLogicCtrl(CMainFrame* pMainFrame)
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
// String Encoder
// --------------------------------------------------------------------------

void CMainFrame::OnStringEncoder()
{
    const Application* application;
    LogicCtrl* logic_ctrl;
    std::tie(application, logic_ctrl) = GetActiveApplicationAndLogicCtrl(this);

    const LogicSettings* logic_settings = nullptr;
    std::string initial_text;

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
    std::string initial_path;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) == 1 )
    {
        lexer_language = Lexers::GetLexer_Logic(*application);
        initial_path = application->GetApplicationFilePath();
    }

    else
    {
        // if no application is available, it means that a form file is open; in that case, use the user default logic settings
        lexer_language = Lexers::GetLexer_Logic(LogicSettings::GetUserDefaultSettings());

        CDocument* document;
        std::tie(document, std::ignore) = GetActiveDocumentAndLogicCtrl(this);

        if( document != nullptr )
            initial_path = TC::ToUtf8(document->GetPathName());
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
