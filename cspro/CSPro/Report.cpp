#include "StdAfx.h"
#include "MainFrm.h"
#include <zDesignerF/TextTemplatePreviewer.h>


const TextSource* CMainFrame::GetHtmlOrDerivableReportTextSourceCurrentlyEditing(std::string* const report_name_for_report_preview)
{
    const TextSource* report_text_source = nullptr;

    CMDIChildWnd* const pActiveWnd = MDIGetActive();
    CView* const pActiveView = ( pActiveWnd != nullptr ) ? pActiveWnd->GetActiveView() :
                                                           nullptr;

    if( pActiveWnd != nullptr )
    {
        // returns true if the text source should be updated
        auto set_report_text_source = [&](const auto* id, auto get_name)
        {
            const FileExtensionAnalyzer report_extension_analyser(id->GetTextSource()->GetFilePath());

            if( report_extension_analyser.IsTypeHtmlOrDerivable() )
            {
                report_text_source = id->GetTextSource();

                if( report_name_for_report_preview != nullptr )
                {
                    *report_name_for_report_preview = UTF8_TODO::GetUtf8(get_name());
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


void CMainFrame::OnUpdateViewReportPreview(CCmdUI* const pCmdUI)
{
    const TextSource* const report_text_source = GetHtmlOrDerivableReportTextSourceCurrentlyEditing(nullptr);
    pCmdUI->Enable(( report_text_source != nullptr ));
}


void CMainFrame::OnViewReportPreview()
{
    Application* application;
    std::string report_name;
    const TextSource* const report_text_source = GetHtmlOrDerivableReportTextSourceCurrentlyEditing(&report_name);

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) != 1 )
        return;

    ASSERT(application != nullptr && report_text_source != nullptr);

    try
    {
        TextTemplatePreviewer text_template_previewer(report_text_source->GetFilePath(),
                                                      report_text_source->GetText(),
                                                      application->GetLogicSettings());

        // view the report, using an ExceptionHolder to display uncaught exceptions from the Action Invoker
        Viewer viewer;
        viewer.UseEmbeddedViewer()
              .UseExceptionHolder(nullptr)
              .SetTitle("Report Preview: " + report_name)
              .ViewHtmlUrl(text_template_previewer.GetUrl());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
