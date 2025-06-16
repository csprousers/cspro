#include "StdAfx.h"
#include "Markdown.h"
#include "ParserCallback.h"
#include <zHtml/HtmlWriter.h>
#include <external/md4c/md4c-html.h>


namespace
{
    static constexpr unsigned ParserFlags = MD_DIALECT_GITHUB; // MD_FLAG_... combinations
    static constexpr unsigned HtmlRendererFlags = 0;           // MD_HTML_FLAG_... combinations
}


void Markdown::AddToStringCallback(const char* const text, const unsigned int text_size, void* const userdata)
{
    static_cast<std::string*>(userdata)->append(text, text_size);
}


void Markdown::ToHtml(std::string& html, const std::string_view markdown_sv)
{
    const int result = md_html(markdown_sv.data(), markdown_sv.length(),
                               AddToStringCallback, &html,
                               ParserFlags, HtmlRendererFlags);

    if( result != 0 )
        throw CSProException("There was an error creating HTML from the Markdown.");
}


std::string Markdown::ToHtml(const std::string_view markdown_sv)
{
    std::string html;
    ToHtml(html, markdown_sv);
    return html;
}


std::string Markdown::ToHtmlDocument(const std::string_view title_sv, const std::string_view markdown_sv,
                                     CssProvider* const css_provider/* = nullptr*/)
{
    return ToHtmlDocument(title_sv, css_provider, [&](std::string& html) { ToHtml(html, markdown_sv); });
}


std::string Markdown::ToHtmlDocumentFromConvertedMarkdown(const std::string_view title_sv, const std::string_view html_sv,
                                                          CssProvider* const css_provider/* = nullptr*/)

{
    return ToHtmlDocument(title_sv, css_provider, [&](std::string& html) { html.append(html_sv); });
}



template<typename CF>
std::string Markdown::ToHtmlDocument(std::string_view title_sv, CssProvider* const css_provider, const CF& callback_function)
{
    std::string html(
R"!(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>)!");

    html.append(Encoders::ToHtml(title_sv))
        .append("</title>\n");

    if( css_provider != nullptr )
        html.append(css_provider->GetCssForHead());

    html.append(
R"!(<style>
.markdown-body {
    box-sizing: border-box;
    min-width: 200px;
    max-width: 980px;
    margin: 0 auto;
    padding: 45px;
}

@media (max-width: 767px) {
    .markdown-body {
        padding: 15px;
    }
}
</style>
</head>
<body>
<article class="markdown-body">
)!");

    callback_function(html);

    html.append(
R"!(</article>
</body>
</html>
)!");

    return html;
}


struct Markdown::ParserCallbackWrapper
{
    ParserCallback& parser_callback;
    std::exception_ptr thrown_exception;

    template<typename CF, typename... Args>
    static int Run(void* const userdata, CF callback_function, Args&&... args);

    static int enter_block_callback(MD_BLOCKTYPE type, void* const detail, void* const userdata);
    static int leave_block_callback(MD_BLOCKTYPE type, void* const detail, void* const userdata);
    static int enter_span_callback(MD_SPANTYPE type, void* const detail, void* const userdata);
    static int leave_span_callback(MD_SPANTYPE type, void* const detail, void* const userdata);
    static int process_output_callback(MD_TEXTTYPE type, const char* text, unsigned int size, void* const userdata);
};


void Markdown::Parse(ParserCallback& parser_callback, const std::string_view markdown_sv)
{
    const MD_PARSER parser
    {
        0,
        ParserFlags,
        ParserCallbackWrapper::enter_block_callback,
        ParserCallbackWrapper::leave_block_callback,
        ParserCallbackWrapper::enter_span_callback,
        ParserCallbackWrapper::leave_span_callback,
        ParserCallbackWrapper::process_output_callback,
        nullptr,
        nullptr
    };

    ParserCallbackWrapper parser_callback_wrapper { parser_callback };

    const int result = md_parse(markdown_sv.data(), markdown_sv.length(), &parser, &parser_callback_wrapper);

    if( parser_callback_wrapper.thrown_exception )
    {
        ASSERT(result != 0);
        std::rethrow_exception(parser_callback_wrapper.thrown_exception);
    }

    if( result != 0 )
        throw CSProException("There was an error parsing the Markdown.");
}


template<typename CF, typename... Args>
int Markdown::ParserCallbackWrapper::Run(void* const userdata, CF callback_function, Args&&... args)
{
    ASSERT(userdata != nullptr);
    ParserCallbackWrapper& parser_callback_wrapper = *static_cast<ParserCallbackWrapper*>(userdata);

    try
    {
        (parser_callback_wrapper.parser_callback.*callback_function)(std::forward<Args>(args)...);
        return 0;
    }

    catch(...)
    {
        parser_callback_wrapper.thrown_exception = std::current_exception();
        return -1;
    }
}


int Markdown::ParserCallbackWrapper::enter_block_callback(const MD_BLOCKTYPE type, void* const detail, void* const userdata)
{
    return Run(userdata, &Markdown::ParserCallback::EnterBlock, type, detail);
}


int Markdown::ParserCallbackWrapper::leave_block_callback(const MD_BLOCKTYPE type, void* const detail, void* const userdata)
{
    return Run(userdata, &Markdown::ParserCallback::LeaveBlock, type, detail);
}


int Markdown::ParserCallbackWrapper::enter_span_callback(const MD_SPANTYPE type, void* const detail, void* const userdata)
{
    return Run(userdata, &Markdown::ParserCallback::EnterSpan, type, detail);
}


int Markdown::ParserCallbackWrapper::leave_span_callback(const MD_SPANTYPE type, void* const detail, void* const userdata)
{
    return Run(userdata, &Markdown::ParserCallback::LeaveSpan, type, detail);
}


int Markdown::ParserCallbackWrapper::process_output_callback(const MD_TEXTTYPE type, const char* const text, unsigned int size, void* const userdata)
{
    return Run(userdata, &Markdown::ParserCallback::ProcessOutput, type, std::string_view(text, size));
}
