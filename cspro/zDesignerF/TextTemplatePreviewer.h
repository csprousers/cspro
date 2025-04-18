#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/UriResolver.h>

class LogicSettings;
struct TextTemplateToken;


// --------------------------------------------------------------------------
// TextTemplatePreviewer is used to show a preview of HTML / Markdown
// text templates.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF TextTemplatePreviewer
{
public:
    // CSProException exceptions are thrown if the text template does not compile.
    TextTemplatePreviewer(std::string text_template_file_path, std::string_view text_template_sv,
                          const LogicSettings& logic_settings, const char* action = "previewing");
    ~TextTemplatePreviewer();

    SharableString GetHtml() const { return m_html; }

    std::string GetUrl();
    std::unique_ptr<UriResolver> GetUriResolver();

private:
    std::string CreateHtmlForHtml(const std::vector<TextTemplateToken>& tokens) const;
    std::string CreateHtmlForMarkdown(const std::vector<TextTemplateToken>& tokens) const;

private:
    class DesignerTextTemplateTokenizer;
    struct VirtualFileMappingDetails;

    std::string m_textTemplateFilePath;
    int m_lexerLanguage;
    SharableString m_html;
    std::unique_ptr<VirtualFileMappingDetails> m_virtualFileMappingDetails;
};
