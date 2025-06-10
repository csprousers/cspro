#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/UriResolver.h>
#include <zLogicO/TextTemplateTokenizer.h>

class CapiText;
enum class EncodeType : int;
class LogicSettings;


// --------------------------------------------------------------------------
// TextTemplatePreviewer is used to show a preview of HTML / Markdown
// text templates (question text and reports).
//
// The constructors throw CSProException exceptions if the text template does
// not compile.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF TextTemplatePreviewer
{
private:
    struct ConstructionData;
    struct VirtualFileMappingDetails;

    TextTemplatePreviewer(EncodeType encode_type, const LogicSettings& logic_settings,
                          std::string_view text_template_sv, std::optional<std::string> text_template_file_path,
                          const char* action = "previewing");

public:
    // Creates a preview of a text template.
    // If previewing Markdown, a HTML document is constructed.
    TextTemplatePreviewer(const std::string& text_template_file_path, std::string_view text_template_sv,
                          const LogicSettings& logic_settings, const char* action = "previewing");

    // Creates a preview of question text, constructing only the HTML, not the full page.
    TextTemplatePreviewer(const CapiText& capi_text, const LogicSettings& logic_settings);

    ~TextTemplatePreviewer();

    SharableString GetHtml() const { return m_html; }

    std::string GetUrl();
    std::unique_ptr<UriResolver> GetUriResolver();

private:
    static EncodeType GetEncodeType(const std::string& text_template_file_path);

    // Instantiates the tokenizer and tokenizes the text template.
    static void TokenizeTemplate(ConstructionData& data, std::string_view text_template_sv);

    static constexpr std::tuple<const char*, const char*> GetDelimiters(TextTemplateToken::Type type);
    static constexpr std::tuple<const char*, const char*> GetEscapedDelimiters(TextTemplateToken::Type type);

    static void AppendColorizedLogic(std::string& html, TextTemplateToken::Type type, const std::string& colorized_tag_html);

    static std::string ProcessHtml(ConstructionData& data);

    std::string ProcessMarkdown(ConstructionData& data) const;
    static std::string ProcessMarkdownWithNoHtmlTags(ConstructionData& data);
    static std::string ProcessMarkdownWithHtmlTagSupport(ConstructionData& data);

    template<typename T>
    static bool DirectTextContains(ConstructionData& data, const T& text);

private:
    std::optional<std::string> m_textTemplateFilePath;
    SharableString m_html;
    std::unique_ptr<VirtualFileMappingDetails> m_virtualFileMappingDetails;
};
