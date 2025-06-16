#include "StdAfx.h"
#include "CSDocCompilerSettings.h"


CSDocCompilerSettingsForCSDocumentPreview::CSDocCompilerSettingsForCSDocumentPreview(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, BuildWnd* const build_wnd)
    :   CSDocCompilerSettings(std::move(doc_set_spec)),
        m_fileServer(assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer()),
        m_buildWnd(build_wnd)
{
}


void CSDocCompilerSettingsForCSDocumentPreview::AddCompilerMessage(const CompilerMessageType compiler_message_type, const std::string& text)
{
    if( m_buildWnd != nullptr )
        m_buildWnd->AddMessage(compiler_message_type, text);
}


std::string CSDocCompilerSettingsForCSDocumentPreview::GetStylesheetsHtml()
{
    static const std::string stylesheets_html = GetStylesheetLinkHtml(m_fileServer.CreateFileUrl(GetStylesheetCssFilePath(CSDocStylesheetFilename)));
    return stylesheets_html;
}


std::string CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForTitle(const std::string& path)
{
    return CreateUrlForTopic(SO::Empty_string, path);
}


std::string CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForTopic(const std::string& project, const std::string& path)
{
    ASSERT(PortableFunctions::FileIsRegular(path));

    std::string onclick_html = "!window.chrome.webview.postMessage({action:\"open\",path:" + Encoders::ToJsonString(path);

    if( project.empty() )
    {
        onclick_html.append(",docSet:")
                    .append(IntToString(reinterpret_cast<int64_t>(m_docSetSpec.get())));
    }

    onclick_html.append("});return false;");

    return onclick_html;
}


std::string CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForLogicTopic(const char* const help_topic_filename)
{
    // if there is a CSPro project related to this Document Set, link to those documents
    if( m_logicHelpsArePartOfProject.value_or(true) )
    {
        try
        {
            return CreateUrlForLogicHelpTopicInCSProProject(help_topic_filename);
        }

        catch(...)
        {
            m_logicHelpsArePartOfProject = false;
        }
    }

    // otherwise link to the online helps
    return CreateUrlForLogicTopicOnCSProUsersWebsite(help_topic_filename);
}


std::string CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForImageFile(const std::string& path)
{
    return m_fileServer.CreateFileUrl(path);
}


std::optional<unsigned> CSDocCompilerSettingsForCSDocumentPreview::GetContextId(const std::string& context, const bool use_if_exists)
{
    std::optional<unsigned> context_id = CSDocCompilerSettings::GetContextId(context, use_if_exists);

    if( !context_id.has_value() && !use_if_exists )
        AddCompilerMessage(CompilerMessageType::Warning, FormatText("The context '%s' is unknown.", context.c_str()));

    return context_id;
}
