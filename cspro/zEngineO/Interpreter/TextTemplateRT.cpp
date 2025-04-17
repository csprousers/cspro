#include "stdafx.h"
#include "IncludesRT.h"
#include "Report.h"
#include "StringWriter.h"
#include "Nodes/TextTemplate.h"


SharableString LogicInterpreter::EncodeText(SharableString text, EncodeType encode_type)
{
    if( encode_type == EncodeType::Default )
        encode_type = m_currentEncodeType;

    std::unique_ptr<std::string> encoded_text;

    switch( encode_type )
    {
        case EncodeType::Html:
            encoded_text = Encoders::ToHtmlWorker(*text);
            break;

        case EncodeType::Csv:
            encoded_text = Encoders::ToCsvWorker(*text);
            break;

        case EncodeType::PercentEncoding:
            encoded_text = Encoders::ToPercentEncodingWorker(*text);
            break;

        case EncodeType::Uri:
            encoded_text = Encoders::ToUriWorker(*text);
            break;

        case EncodeType::UriComponent:
            encoded_text = Encoders::ToUriComponentWorker(*text);
            break;

        case EncodeType::Slashes:
            return Encoders::ToEscapedString(text.Release());

        case EncodeType::JsonString:
            return Encoders::ToJsonString(*text);

        case EncodeType::Markdown:
            encoded_text = Encoders::ToMarkdownWorker(*text);
            break;

        default:
            ASSERT(false);
            break;
    }

    if( encoded_text != nullptr )
        return std::move(encoded_text);

    return std::move(text);
}


SharableString LogicInterpreter::EncodeText(SharableString text, const Symbol& symbol)
{
    if( symbol.IsA(SymbolType::Report) )
    {
        EncodeType encode_type;

        switch( assert_cast<const Report&>(symbol).GetEscapeType() )
        {
            case ReportFile::EscapeType::Html:
                encode_type = EncodeType::Html;
                break;

            case ReportFile::EscapeType::Markdown:
                encode_type = EncodeType::Markdown;
                break;

            case ReportFile::EscapeType::Csv:
                encode_type = EncodeType::Csv;
                break;

            default:
                // no encoding if the Report does not define an escape type
                ASSERT(assert_cast<const Report&>(symbol).GetEscapeType() == ReportFile::EscapeType::None);
                return text;
        }

        return EncodeText(std::move(text), encode_type);
    }

    else if( symbol.IsA(SymbolType::StringWriter) )
    {
        const StringWriter& string_writer = assert_cast<const StringWriter&>(symbol);
        ASSERT(std::holds_alternative<std::string>(string_writer.GetOutput()));

        return EncodeText(std::move(text), string_writer.GetEncodeType());
    }

    else
    {
        ASSERT(false);
        return text;
    }
}


double LogicInterpreter::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine(const int program_index)
{
    const auto& text_template_node = GetNode<Nodes::TextTemplate>(program_index);
    Symbol* symbol = &NPT_Ref(text_template_node.symbol_index);
    std::string* text_builder = nullptr;

    if( symbol->IsA(SymbolType::StringWriter) )
    {
        std::variant<std::string, int>& output = assert_cast<StringWriter&>(*symbol).GetOutput();

        if( std::holds_alternative<int>(output) )
        {
            symbol = &NPT_Ref(std::get<int>(output));
            ASSERT(symbol->IsA(SymbolType::Report));
        }

        else
        {
            text_builder = &std::get<std::string>(output);
        }
    }

    if( symbol->IsA(SymbolType::Report) )
    {
        text_builder = GetReportTextBuilderWithValidityCheck(assert_cast<Report&>(*symbol));;

        if( text_builder == nullptr )
            return 0;
    }

    ASSERT(text_builder != nullptr);

    // write out direct text...
    if( text_template_node.type == Nodes::TextTemplate::Type::DirectText )
    {
        text_builder->append(*m_engineData->string_literals[text_template_node.expression]);
    }

    // ...or the results of a text fill...
    else if( text_template_node.type == Nodes::TextTemplate::Type::TextFill )
    {
        SharableString fill_text = EvaluateTextFill(text_template_node.expression);

        if( text_template_node.encode_text == 1 )
            fill_text = EncodeText(std::move(fill_text), *symbol);

        text_builder->append(*fill_text);
    }

    // ...or the results of a symbol.write / writeEncoded / writeEncodedLine / writeLine call
    else
    {
        ASSERT(text_template_node.type == Nodes::TextTemplate::Type::Write);

        SharableString fill_text = EvaluateUserMessage(text_template_node.expression, text_template_node.function_code);

        if( text_template_node.encode_text == 1 )
        {
            ASSERT(text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITEENCODED_CODE ||
                   text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITEENCODEDLINE_CODE);

            if( text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITEENCODEDLINE_CODE )
                fill_text.MakeModifiable().push_back('\n');

            fill_text = EncodeText(std::move(fill_text), *symbol);
        }

        text_builder->append(*fill_text);

        if( text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITELINE_CODE )
            text_builder->push_back('\n');
    }

    return 1;
}
