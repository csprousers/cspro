#include "StdAfx.h"
#include "CSDocCompilerSettings.h"


CSDocCompilerSettingsForCSDocumentPreview::CSDocCompilerSettingsForCSDocumentPreview(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, BuildWnd* build_wnd)
    :   CSDocCompilerSettings(std::move(doc_set_spec)),
        m_fileServer(assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer()),
        m_buildWnd(build_wnd)
{
}


void CSDocCompilerSettingsForCSDocumentPreview::AddCompilerMessage(const CompilerMessageType compiler_message_type, const std::wstring& text)
{
    if( m_buildWnd != nullptr )
        m_buildWnd->AddMessage(compiler_message_type, text);
}


std::wstring CSDocCompilerSettingsForCSDocumentPreview::GetStylesheetsHtml()
{
    static std::wstring stylesheets_html = GetStylesheetLinkHtml(m_fileServer.GetFilenameUrl(GetStylesheetCssPath(CSDocStylesheetFilename)));
    return stylesheets_html;
}


std::wstring CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForTitle(const std::wstring& path)
{
    return CreateUrlForTopic(SO::EmptyString, path);
}


std::wstring CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForTopic(const std::wstring& project, const std::wstring& path)
{
    ASSERT(PortableFunctions::FileIsRegular(path));

    std::wstring onclick_html = _T("!window.chrome.webview.postMessage({action:\"open\",path:") + Encoders::ToJsonString(path);

    if( project.empty() )
        onclick_html.append(_T(",docSet:") + IntToString(reinterpret_cast<int64_t>(m_docSetSpec.get())));

    return onclick_html + _T("});return false;");
}


std::wstring CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForLogicTopic(const TCHAR* help_topic_filename)
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
    return CreateUrlForLogicTopicOnCSProUsersForum(help_topic_filename);
}


std::wstring CSDocCompilerSettingsForCSDocumentPreview::CreateUrlForImageFile(const std::wstring& path)
{
    return m_fileServer.GetFilenameUrl(path);
}


std::optional<unsigned> CSDocCompilerSettingsForCSDocumentPreview::GetContextId(const std::wstring& context, const bool use_if_exists)
{
    std::optional<unsigned> context_id = CSDocCompilerSettings::GetContextId(context, use_if_exists);

    if( !context_id.has_value() && !use_if_exists )
        AddCompilerMessage(CompilerMessageType::Warning, FormatTextCS2WS(_T("The context '%s' is unknown."), context.c_str()));

    return context_id;
}
