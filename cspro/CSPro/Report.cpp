#include "StdAfx.h"
#include "MainFrm.h"
#include "ReportPropertiesDlg.h"
#include <zDesignerF/ReportPreviewer.h>


LRESULT CMainFrame::OnEditReportProperties(WPARAM wParam, LPARAM lParam)
{
    auto report_named_text_source = reinterpret_cast<NamedTextSource*>(wParam);
    auto pDoc = reinterpret_cast<CDocument*>(lParam);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

    if( pAplDoc != nullptr )
    {
        ReportPropertiesDlg report_properties_dlg(*pAplDoc, *pDoc, *report_named_text_source);

        if( report_properties_dlg.DoModal() == IDOK )
        {
            pAplDoc->SetModifiedFlag();
            return 1;
        }
    }

    return 0;
}


const TextSource* CMainFrame::GetHtmlReportTextSourceCurrentlyEditing(std::wstring* report_name_for_report_preview)
{
    const TextSource* report_text_source = nullptr;

    CMDIChildWnd* pActiveWnd = MDIGetActive();
    CView* pActiveView = ( pActiveWnd != nullptr ) ? pActiveWnd->GetActiveView() : nullptr;

    if( pActiveWnd != nullptr )
    {
        // returns true if the text source should be updated
        auto set_report_text_source = [&](const auto* id, auto get_name)
        {
            if( FileExtensions::IsFilenameHtml(id->GetTextSource()->GetFilename()) )
            {
                report_text_source = id->GetTextSource();

                if( report_name_for_report_preview != nullptr )
                {
                    *report_name_for_report_preview = get_name();
                    return true;
                }
            }

            return false;
        };

        if( pActiveView->IsKindOf(RUNTIME_CLASS(CFSourceEditView)) )
        {
            CFormID* pFormID = GetNodeIdForSourceCode<CFormID>();

            if( pFormID != nullptr && pFormID->GetItemType() == eFFT_REPORT && set_report_text_source(pFormID, [&]() { return CS2WS(ValueOrDefault(pFormID->GetName())); }) )
                PutSourceCode(pFormID, false);
        }

        else if( pActiveView->IsKindOf(RUNTIME_CLASS(CLogicView)) )
        {
            AppTreeNode* app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();

            if( app_tree_node != nullptr && app_tree_node->GetAppFileType() == AppFileType::Report && set_report_text_source(app_tree_node, [&]() { return app_tree_node->GetName(); }) )
                PutOSourceCode(app_tree_node, false);
        }
    }

    return report_text_source;
}


void CMainFrame::OnUpdateViewReportPreview(CCmdUI* pCmdUI)
{
    const TextSource* report_text_source = GetHtmlReportTextSourceCurrentlyEditing(nullptr);
    pCmdUI->Enable(( report_text_source != nullptr ));
}


void CMainFrame::OnViewReportPreview()
{
    Application* application;
    std::wstring report_name;
    const TextSource* report_text_source = GetHtmlReportTextSourceCurrentlyEditing(&report_name);

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) != 1 )
        return;

    ASSERT(application != nullptr && report_text_source != nullptr);

    try
    {
        ReportPreviewer report_previewer(report_text_source->GetText(), application->GetLogicSettings());

        // view the report
        Viewer viewer;
        viewer.UseEmbeddedViewer()
              .SetTitle(_T("Report Preview: ") + report_name)
              .ViewHtmlUrl(report_previewer.GetReportUrl(report_text_source->GetFilename()));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
