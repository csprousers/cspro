#pragma once

#include <zDocument/CSDocCompilerSettings.h>


class CSDocCompilerSettingsForCSDocumentPreview : public CSDocCompilerSettings
{
public:
    CSDocCompilerSettingsForCSDocumentPreview(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, BuildWnd* build_wnd);

    void AddCompilerMessage(CompilerMessageType compiler_message_type, const std::string& text) override;

    std::string GetStylesheetsHtml() override;

    std::string CreateUrlForTitle(const std::string& path) override;
    std::string CreateUrlForTopic(const std::string& project, const std::string& path) override;
    std::string CreateUrlForLogicTopic(const char* help_topic_filename) override;
    std::string CreateUrlForImageFile(const std::string& path) override;

    std::optional<unsigned> GetContextId(const std::string& context, bool use_if_exists) override;

private:
    SharedHtmlLocalFileServer& m_fileServer;
    BuildWnd* const m_buildWnd;
    std::optional<bool> m_logicHelpsArePartOfProject;
};
