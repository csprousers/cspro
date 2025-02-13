#include "StdAfx.h"
#include "DocSetBuildHandlerFrame.h"
#include "DocSetBuilder.h"
#include "GenerateDlg.h"


BEGIN_MESSAGE_MAP(DocSetBuildHandlerFrame, TextEditFrame)

    ON_COMMAND(ID_COMPILE_DOCSET_COMPLETE, OnCompileDocumentSet)
    ON_UPDATE_COMMAND_UI(ID_COMPILE_DOCSET_COMPLETE, OnUpdateDocumentSetMustExist)
    ON_COMMAND(ID_EXPORT_DOCSET, OnExportDocumentSet)
    ON_UPDATE_COMMAND_UI(ID_EXPORT_DOCSET, OnUpdateDocumentSetMustExist)

    ON_MESSAGE(UWM::CSDocument::ShowHtmlAndRestoreScrollbarState, OnShowHtmlAndRestoreScrollbarState)

END_MESSAGE_MAP()


void DocSetBuildHandlerFrame::PopulateBuildMenu(CMenu& popup_menu)
{
    DynamicMenuBuilder dynamic_menu_builder(popup_menu, ID_BUILD_PLACEHOLDER);

    AddFrameSpecificItemsToBuildMenu(dynamic_menu_builder);

    const DocSetSpec* doc_set_spec = GetTextEditDoc().GetAssociatedDocSetSpec();

    if( doc_set_spec == nullptr )
        return;

    dynamic_menu_builder.AddSeparator();
    dynamic_menu_builder.AddOption(ID_COMPILE_DOCSET_COMPLETE, _T("Compile Entire Document Set\tCtrl+Shift+K"));

#ifdef HELP_TODO_RESTORE_FOR_CSPRO81
    dynamic_menu_builder.AddSeparator();
    dynamic_menu_builder.AddOption(ID_EXPORT_DOCSET, _T("E&xport Document Set..."));
#endif
}


void DocSetBuildHandlerFrame::ShowHtmlAndRestoreScrollbarState(HtmlViewCtrl& html_view_ctrl, std::wstring url)
{
    ASSERT(!url.empty());

    const bool refreshing_current_source = ( !std::get<0>(m_currentUrlAndScrollbarStateToRestore).empty() &&
                                             std::get<0>(m_currentUrlAndScrollbarStateToRestore) == html_view_ctrl.GetSource() );

    m_currentUrlAndScrollbarStateToRestore = std::make_tuple(std::move(url), 0);

    // if refreshing the current source, get the current scrollbar position before navigating to the new page
    if( refreshing_current_source )
    {
        html_view_ctrl.ExecuteScript(_T("window.pageYOffset"),
            [&](const std::wstring& result)
            {
                try
                {
                    std::get<1>(m_currentUrlAndScrollbarStateToRestore) = std::stoi(result);
                }
                catch(...) { ASSERT(false); }

                PostMessage(UWM::CSDocument::ShowHtmlAndRestoreScrollbarState, reinterpret_cast<WPARAM>(&html_view_ctrl));
            });
    }

    else
    {
        PostMessage(UWM::CSDocument::ShowHtmlAndRestoreScrollbarState, reinterpret_cast<WPARAM>(&html_view_ctrl));
    }
}


LRESULT DocSetBuildHandlerFrame::OnShowHtmlAndRestoreScrollbarState(WPARAM wParam, LPARAM /*lParam*/)
{
    ASSERT(!std::get<0>(m_currentUrlAndScrollbarStateToRestore).empty());

    HtmlViewCtrl& html_view_ctrl = *reinterpret_cast<HtmlViewCtrl*>(wParam);

    html_view_ctrl.NavigateTo(std::get<0>(m_currentUrlAndScrollbarStateToRestore));

    // if specified, restore the scrollbar position
    if( std::get<1>(m_currentUrlAndScrollbarStateToRestore) != 0 )
        html_view_ctrl.ExecuteScript(FormatText(_T("window.scrollTo(0, %d);"), static_cast<int>(std::get<1>(m_currentUrlAndScrollbarStateToRestore))));

    return 1;
}


void DocSetBuildHandlerFrame::OnUpdateDocumentSetMustExist(CCmdUI* pCmdUI)
{
    const DocSetSpec* doc_set_spec = GetTextEditDoc().GetAssociatedDocSetSpec();
    pCmdUI->Enable(doc_set_spec != nullptr);
}


void DocSetBuildHandlerFrame::OnCompileDocumentSet()
{
    DocSetSpec* doc_set_spec = GetTextEditDoc().GetAssociatedDocSetSpec();
    ASSERT(doc_set_spec != nullptr);

    DocSetBuilderCompileAllGenerateTask generate_task(doc_set_spec);

    GenerateDlg generate_dlg(generate_task, this);
    generate_dlg.DoModal();

    // show any compilation errors in the Build window
    if( generate_task.GetDocumentsWithCompilationErrors().empty() )
        return;

    CSDocumentBuildWnd* build_wnd = GetMainFrame().GetBuildWnd();

    if( build_wnd == nullptr )
        return;

    build_wnd->Initialize(nullptr, doc_set_spec->GetFilename(), _T("Compilation Error Display"));

    for( const auto& [filename, error] : generate_task.GetDocumentsWithCompilationErrors() )
        build_wnd->AddError(filename, error);

    build_wnd->Finalize();
}


void DocSetBuildHandlerFrame::OnExportDocumentSet()
{
    AfxMessageBox(L"HELP_TODO: OnExportDocumentSet");
}
