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

        switch( assert_cast<const Report&>(symbol).GetEncoding() )
        {
            case ReportFile::Encoding::Html:
                encode_type = EncodeType::Html;
                break;

            case ReportFile::Encoding::Markdown:
                encode_type = EncodeType::Markdown;
                break;

            case ReportFile::Encoding::Csv:
                encode_type = EncodeType::Csv;
                break;

            default:
                // no encoding if the Report does not define an encoding
                ASSERT(assert_cast<const Report&>(symbol).GetEncoding() == ReportFile::Encoding::None);
                return text;
        }

        return EncodeText(std::move(text), encode_type);
    }

    else if( symbol.IsA(SymbolType::StringWriter) )
    {
        const StringWriter& string_writer = assert_cast<const StringWriter&>(symbol);
        ASSERT(std::holds_alternative<SharableString>(string_writer.GetOutput()));

        // no encoding if the StringWriter does not define an encoding
        if( string_writer.GetEncodeType() == EncodeType::Default )
            return text;

        return EncodeText(std::move(text), string_writer.GetEncodeType());
    }

    else
    {
        ASSERT(false);
        return text;
    }
}


std::tuple<Symbol*, std::string*> LogicInterpreter::GetTextTemplateBuilder(Symbol& symbol)
{
    if( symbol.IsA(SymbolType::Report) )
    {
        Report& report = assert_cast<Report&>(symbol);
        std::string* const report_text_builder = report.GetReportTextBuilder();

        if( report_text_builder == nullptr )
            IssueMessage(MessageType::Error, 48111, report.GetName().c_str(), "The report creation has not yet been initiated.");

        return { &symbol, report_text_builder };
    }

    else if( symbol.IsA(SymbolType::StringWriter) )
    {
        std::variant<SharableString, int>& output = assert_cast<StringWriter&>(symbol).GetOutput();

        if( std::holds_alternative<int>(output) )
        {
            return GetTextTemplateBuilder(NPT_Ref(std::get<int>(output)));
        }

        else
        {
            ASSERT(std::holds_alternative<SharableString>(output));
            return { &symbol, &std::get<SharableString>(output).MakeModifiable() };
        }
    }

    else
    {
        return ReturnProgrammingError(std::tuple<Symbol*, std::string*>());
    }
}


Engine::Value LogicInterpreter::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine(const int program_index)
{
    const auto& text_template_node = GetNode<Nodes::TextTemplate>(program_index);
    Symbol& specified_symbol = NPT_Ref(text_template_node.symbol_index);

    const Symbol* underying_text_template_symbol;
    std::string* text_builder;
    std::tie(underying_text_template_symbol, text_builder) = GetTextTemplateBuilder(specified_symbol);

    if( text_builder == nullptr )
        return Engine::Value::Bool(false);

    ASSERT(underying_text_template_symbol != nullptr);

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
            fill_text = EncodeText(std::move(fill_text), *underying_text_template_symbol);

        text_builder->append(*fill_text);
    }

    // ...or the results of a write / writeEncoded / writeEncodedLine / writeLine call
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

            fill_text = EncodeText(std::move(fill_text), *underying_text_template_symbol);
        }

        text_builder->append(*fill_text);

        if( text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITELINE_CODE )
            text_builder->push_back('\n');
    }

    return Engine::Value::Bool(true);
}
