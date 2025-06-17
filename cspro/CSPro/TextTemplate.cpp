#include "StdAfx.h"
#include "MainFrm.h"
#include <zFormF/QSFEView.h>
#include <zCapiO/QSFView.h>
#include <zDesignerF/TextTemplatePreviewer.h>


std::variant<std::monostate,
             const CapiText*,
             const TextSource*> CMainFrame::GetHtmlOrDerivableTextTemplateCurrentlyEditing(std::string* const report_name_for_report_preview)
{
    CMDIChildWnd* const pActiveWnd = MDIGetActive();
    CView* const pActiveView = ( pActiveWnd != nullptr ) ? pActiveWnd->GetActiveView() :
                                                           nullptr;

    if( pActiveWnd == nullptr )
        return std::monostate();

    // handle question text
    if( pActiveView->IsKindOf(RUNTIME_CLASS(CQSFEView)) )
    {
        const CQSFEView& qsf_editor_view = assert_cast<CQSFEView&>(*pActiveView);
        return &qsf_editor_view.GetCurrentCapiText();
    }

    // handle reports
    const TextSource* report_text_source = nullptr;

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

    if( report_text_source != nullptr )
        return report_text_source;

    return std::monostate();
}


void CMainFrame::OnUpdateViewPreviewTextTemplate(CCmdUI* const pCmdUI)
{
    const std::variant<std::monostate,
                       const CapiText*,
                       const TextSource*> text_template = GetHtmlOrDerivableTextTemplateCurrentlyEditing(nullptr);

    pCmdUI->Enable(!std::holds_alternative<std::monostate>(text_template));
}


void CMainFrame::OnViewPreviewTextTemplate()
{
    std::string report_name;
    const std::variant<std::monostate,
                       const CapiText*,
                       const TextSource*> text_template = GetHtmlOrDerivableTextTemplateCurrentlyEditing(&report_name);

    if( std::holds_alternative<std::monostate>(text_template) )
    {
        ASSERT(false);
        return;
    }

    // previewing text templates requires the application
    Application* application;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) != 1 )
        return;

    ASSERT(application != nullptr);

    try
    {
        // an ExceptionHolder will be used to to display uncaught exceptions from the Action Invoker
        Viewer viewer;
        viewer.UseEmbeddedViewer()
              .UseExceptionHolder(nullptr);

        // handle question text
        if( std::holds_alternative<const CapiText*>(text_template) )
        {
            ASSERT(std::get<const CapiText*>(text_template) != nullptr);
            ASSERT(application->GetCapiQuestionManager() != nullptr);

            const SharableString html_body = CreateQuestionTextHtmlPreview(*application, *std::get<const CapiText*>(text_template));

            std::string html_document = QSFView::CreateCapiTextHtml(html_body.GetString(),
                                                                    application->GetCapiQuestionManager()->GetRuntimeStylesCss(),
                                                                    QSFView::DefaultBackgroundColor());

            viewer.UseSharedHtmlLocalFileServer()
                  .ViewHtmlContent(std::move(html_document), PortableFunctions::PathGetDirectory(application->GetApplicationFilePath()));
        }

        // handle reports
        else
        {
            ASSERT(std::get<const TextSource*>(text_template) != nullptr);

            TextTemplatePreviewer text_template_previewer(std::get<const TextSource*>(text_template)->GetFilePath(),
                                                          std::get<const TextSource*>(text_template)->GetText(),
                                                          application->GetLogicSettings());

            viewer.SetTitle("Report Preview: " + report_name)
                  .ViewHtmlUrl(text_template_previewer.GetUrl());
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


SharableString CMainFrame::CreateQuestionTextHtmlPreview(const Application& application, const CapiText& capi_text) noexcept
{
    try
    {
        const TextTemplatePreviewer text_template_previewer(capi_text, application.GetLogicSettings());
        return text_template_previewer.GetHtml();
    }

    catch( const CSProException& exception )
    {
        // format the exception as HTML
        return "<b><em>" + Encoders::ToHtml(exception.what()) + "</em></b>";
    }
}
