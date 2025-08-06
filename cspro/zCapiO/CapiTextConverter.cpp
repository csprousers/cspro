#include "StdAfx.h"
#include "CapiTextConverter.h"
#include <zHtml/HtmlConverter.h>
#include <zMarkdown/Markdown.h>
#include <zLogicO/TextTemplateTokenizer.h>
#include <zAppO/LogicSettings.h>


// --------------------------------------------------------------------------
// CapiTextConverter::Worker
// --------------------------------------------------------------------------

class CapiTextConverter::Worker
{
public:
    Worker(LogicSettings logic_settings, CapiText input_capi_text);
    virtual ~Worker() { }

    virtual std::string GetConfirmationMessage() = 0;
    virtual CapiText Convert() = 0;

protected:
    void EnsureTokenized();

    void AddLogicEscapeUseCountToConfirmationMessage(std::string& message);

    std::string ConvertHtmlToMarkdown();
    std::string ConvertMarkdownToHtml();

protected:
    LogicSettings m_logicSettings;
    CapiText m_inputCapiText;
    std::unique_ptr<TextTemplateTokenizer> m_textTemplateTokenizer;
};


#define DeclareWorker(NAME)                            \
    class NAME : public CapiTextConverter::Worker      \
    {                                                  \
    public:                                            \
        using Worker::Worker;                          \
        std::string GetConfirmationMessage() override; \
        CapiText Convert() override;                   \
    };

DeclareWorker(HtmlToReportHtmlCapiTextConverter)
DeclareWorker(HtmlToReportMarkdownCapiTextConverter)
DeclareWorker(ReportHtmlToHtmlCapiTextConverter)
DeclareWorker(ReportHtmlToReportMarkdownCapiTextConverter)
DeclareWorker(ReportMarkdownToHtmlCapiTextConverter)
DeclareWorker(ReportMarkdownToReportHtmlCapiTextConverter)


CapiTextConverter::Worker::Worker(LogicSettings logic_settings, CapiText input_capi_text)
    :   m_logicSettings(std::move(logic_settings)),
        m_inputCapiText(std::move(input_capi_text))
{
}


void CapiTextConverter::Worker::EnsureTokenized()
{
    if( m_textTemplateTokenizer != nullptr )
        return;

    try
    {
        m_textTemplateTokenizer = std::make_unique<ErrorSuppressingTextTemplateTokenizer>(true);
        m_textTemplateTokenizer->Tokenize(m_inputCapiText.GetText().GetString(), m_logicSettings);
    }
    catch(...) { ASSERT(false); }
}


void CapiTextConverter::Worker::AddLogicEscapeUseCountToConfirmationMessage(std::string& message)
{
    // only tokenize the text if there may be a logic escape
    if( m_inputCapiText.GetText()->find("<?") == std::string::npos )
        return;

    EnsureTokenized();

    const size_t use_count = std::count_if(m_textTemplateTokenizer->GetTokens().cbegin(),
                                           m_textTemplateTokenizer->GetTokens().cend(),
                                           [&](const TextTemplateToken& token) { return ( token.type == TextTemplateToken::Type::Logic ); });

    if( use_count != 0 )
    {
        message.append(FormatText("\n\nThis question text currently uses %d logic escape%s!",
                                  static_cast<int>(use_count), PluralizeWord(use_count)));
    }
}


std::string CapiTextConverter::Worker::ConvertHtmlToMarkdown()
{
    EnsureTokenized();

    return m_textTemplateTokenizer->ConvertDirectText(
        [&](std::string& direct_text)
        {
            direct_text = HtmlConverter::ToMarkdown(direct_text);
        });
}


std::string CapiTextConverter::Worker::ConvertMarkdownToHtml()
{
    EnsureTokenized();

    return m_textTemplateTokenizer->ConvertDirectText(
        [&](std::string& direct_text)
        {
            direct_text = Markdown::ToHtml(direct_text);
        });
}



// --------------------------------------------------------------------------
// HtmlToReportHtmlCapiTextConverter
// --------------------------------------------------------------------------

std::string HtmlToReportHtmlCapiTextConverter::GetConfirmationMessage()
{
    return "Are you sure that you want to switch from using the visual HTML editor to the text-based HTML editor?\n\n"
           "With this change, this question text will be processed as a text template. "
           "In addition to processing ~~ and ~~~ fills, logic can be executed when placed in between <? ?> tags.";
}


CapiText HtmlToReportHtmlCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::Html);

    return CapiText(m_inputCapiText.GetText(), CapiText::Format::ReportHtml);
}



// --------------------------------------------------------------------------
// HtmlToReportMarkdownCapiTextConverter
// --------------------------------------------------------------------------

std::string HtmlToReportMarkdownCapiTextConverter::GetConfirmationMessage()
{
    return "Are you sure that you want to switch from using the visual HTML editor to using text-based Markdown?\n\n"
           "With this change, this question text will be converted from HTML to Markdown and will be processed as a text template. "
           "In addition to processing ~~ and ~~~ fills, logic can be executed when placed in between <? ?> tags.";
}


CapiText HtmlToReportMarkdownCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::Html);

    return CapiText(ConvertHtmlToMarkdown(), CapiText::Format::ReportMarkdown);
}



// --------------------------------------------------------------------------
// ReportHtmlToHtmlCapiTextConverter
// --------------------------------------------------------------------------

std::string ReportHtmlToHtmlCapiTextConverter::GetConfirmationMessage()
{
    std::string message = "Are you sure that you want to switch from using the text-based HTML editor to the visual HTML editor?\n\n"
                          "With this change, this question text will no longer be processed as a text template. "
                          "While ~~ and ~~~ fills will be processed, logic placed in between <? ?> tags will no longer be executed.";

    AddLogicEscapeUseCountToConfirmationMessage(message);

    return message;
}


CapiText ReportHtmlToHtmlCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::ReportHtml);

    return CapiText(m_inputCapiText.GetText(), CapiText::Format::Html);
}



// --------------------------------------------------------------------------
// ReportHtmlToReportMarkdownCapiTextConverter
// --------------------------------------------------------------------------

std::string ReportHtmlToReportMarkdownCapiTextConverter::GetConfirmationMessage()
{
    return "Are you sure that you want to switch from using HTML to using Markdown?\n\n"
           "With this change, this question text will be converted from HTML to Markdown.";
}


CapiText ReportHtmlToReportMarkdownCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::ReportHtml);

    return CapiText(ConvertHtmlToMarkdown(), CapiText::Format::ReportMarkdown);
}



// --------------------------------------------------------------------------
// ReportMarkdownToHtmlCapiTextConverter
// --------------------------------------------------------------------------

std::string ReportMarkdownToHtmlCapiTextConverter::GetConfirmationMessage()
{
    std::string message = "Are you sure that you want to switch from using Markdown to using the visual HTML editor?\n\n"
                          "With this change, this question text will be converted from Markdown to HTML and will no longer be processed as a text template. "
                          "While ~~ and ~~~ fills will be processed, logic placed in between <? ?> tags will no longer be executed.";

    AddLogicEscapeUseCountToConfirmationMessage(message);

    return message;
}


CapiText ReportMarkdownToHtmlCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::ReportMarkdown);

    return CapiText(ConvertMarkdownToHtml(), CapiText::Format::Html);
}



// --------------------------------------------------------------------------
// ReportMarkdownToReportHtmlCapiTextConverter
// --------------------------------------------------------------------------

std::string ReportMarkdownToReportHtmlCapiTextConverter::GetConfirmationMessage()
{
    return "Are you sure that you want to switch from using Markdown to using HTML?\n\n"
           "With this change, this question text will be converted from Markdown to HTML.";
}


CapiText ReportMarkdownToReportHtmlCapiTextConverter::Convert()
{
    ASSERT(m_inputCapiText.GetFormat() == CapiText::Format::ReportMarkdown);

    return CapiText(ConvertMarkdownToHtml(), CapiText::Format::ReportHtml);
}



// --------------------------------------------------------------------------
// CapiTextConverter
// --------------------------------------------------------------------------

CapiTextConverter::CapiTextConverter(LogicSettings logic_settings, CapiText input_capi_text, const CapiText::Format output_format)
{
    ASSERT(input_capi_text.GetFormat() != output_format);

    if( input_capi_text.GetFormat() == CapiText::Format::Html )
    {
        if( output_format == CapiText::Format::ReportHtml )
        {
            m_worker = std::make_unique<HtmlToReportHtmlCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }

        else
        {
            ASSERT(output_format == CapiText::Format::ReportMarkdown);
            m_worker = std::make_unique<HtmlToReportMarkdownCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }
    }

    else if( input_capi_text.GetFormat() == CapiText::Format::ReportHtml )
    {
        if( output_format == CapiText::Format::Html )
        {
            m_worker = std::make_unique<ReportHtmlToHtmlCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }

        else
        {
            ASSERT(output_format == CapiText::Format::ReportMarkdown);
            m_worker = std::make_unique<ReportHtmlToReportMarkdownCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }
    }

    else
    {
        ASSERT(input_capi_text.GetFormat() == CapiText::Format::ReportMarkdown);

        if( output_format == CapiText::Format::Html )
        {
            m_worker = std::make_unique<ReportMarkdownToHtmlCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }

        else
        {
            ASSERT(output_format == CapiText::Format::ReportHtml);
            m_worker = std::make_unique<ReportMarkdownToReportHtmlCapiTextConverter>(std::move(logic_settings), std::move(input_capi_text));
        }
    }

    ASSERT(m_worker != nullptr);
}


CapiTextConverter::~CapiTextConverter()
{
}


std::string CapiTextConverter::GetConfirmationMessage()
{
    return m_worker->GetConfirmationMessage();
}


CapiText CapiTextConverter::Convert()
{
    return m_worker->Convert();
}
