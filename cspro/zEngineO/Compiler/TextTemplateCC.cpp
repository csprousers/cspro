#include "stdafx.h"
#include "IncludesCC.h"
#include "Report.h"
#include "StringWriter.h"
#include "TextTemplateTokenizer.h"
#include "Nodes/TextTemplate.h"


namespace
{
    constexpr const char* WriteTypeNamedArgument = "rwt";
}


const Symbol& LogicCompiler::CheckTextTemplateIsCurrentlyAccessible(const Symbol& symbol)
{
    ASSERT(symbol.IsOneOf(SymbolType::Report, SymbolType::StringWriter));

    if( symbol.IsA(SymbolType::Report) )
    {
        // the report being created can only be called from a report's logic or from a user-defined function
        if( !IsCompiling(symbol) && !IsCompiling(SymbolType::UserFunction) )
            IssueError(MGF::TextTemplate_accessed_in_invalid_location_48104, symbol.GetName().c_str());
    }

    else if( symbol.IsA(SymbolType::StringWriter) )
    {
        const std::variant<std::string, int>& output = assert_cast<const StringWriter&>(symbol).GetOutput();

        if( std::holds_alternative<int>(output) )
            return CheckTextTemplateIsCurrentlyAccessible(NPT_Ref(std::get<int>(output)));

        ASSERT(std::holds_alternative<std::string>(output));
    }

    else
    {
        ASSERT(false);
    }

    return symbol;
}


int LogicCompiler::CompileTextTemplateFunctions()
{
    // the write...class of functions are used by Report and StringWriter objects
    // and can be invoked directly by a user to output maketext-style text to the text template,
    // or they can be called under-the-hood (via ConvertTextTemplateToSourceBuffer) to output the template contents

    // compiling: [some symbols].write(...);
    //            [some symbols].writeEncoded(...);
    //            [some symbols].writeEncodedLine(...);
    //            [some symbols].writeLine(...);
    const FunctionCode function_code = CurrentToken.function_details->code;
    const Symbol& specified_symbol = *CurrentToken.symbol;

    CheckTextTemplateIsCurrentlyAccessible(specified_symbol);

    auto& text_template_node = CreateNode<Nodes::TextTemplate>(function_code);
    text_template_node.symbol_index = specified_symbol.GetSymbolIndex();

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // named arguments will be used for the under-the-hood content specification mode
    OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
    int dummy_write_type_argument = -1;

    optional_named_arguments_compiler.AddArgument(WriteTypeNamedArgument, dummy_write_type_argument,
        [&]()
        {
            ASSERT(IsCompiling(specified_symbol));

            NextToken();

            auto get_constant_int = [&]()
            {
                if( Tkn != TOKCTE || !IsNumericConstantInteger() )
                    IssueError(MGF::TextTemplate_unsupported_functionality_48103);

                return static_cast<int>(Tokvalue);
            };

            text_template_node.type = static_cast<Nodes::TextTemplate::Type>(get_constant_int());

            NextToken();
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            if( text_template_node.type == Nodes::TextTemplate::Type::DirectText )
            {
                text_template_node.encode_text = 0;

                NextToken();
                text_template_node.expression = get_constant_int();

                NextToken();
            }

            else if( text_template_node.type == Nodes::TextTemplate::Type::TextFill )
            {
                NextToken();
                text_template_node.encode_text = get_constant_int();

                NextToken();
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                NextToken();
                text_template_node.expression = CompileFillText();
            }

            else
            {
                IssueError(MGF::TextTemplate_unsupported_functionality_48103);
            }

            return 1;
        });

    // if using named arguments, we must read the right parenthesis
    if( optional_named_arguments_compiler.Compile(true) != 0 )
    {
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
    }

    // if not using named arguments, the user is supplying the text directly using a write... function call
    else
    {
        text_template_node.type = Nodes::TextTemplate::Type::Write;

        text_template_node.encode_text = ( text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITEENCODED_CODE ||
                                           text_template_node.function_code == FunctionCode::TEXTTEMPLATEFN_WRITEENCODEDLINE_CODE ) ? 1 : 0;

        text_template_node.expression = CompileMessageFunction(function_code);

        // the right parenthesis is read by CompileMessageFunction
    }

    return GetProgramIndex(text_template_node);
}


namespace
{
    class EngineTextTemplateTokenizer : public TextTemplateTokenizer
    {
    public:
        EngineTextTemplateTokenizer(LogicCompiler& logic_compiler, const bool allow_logic_escapes)
            :   TextTemplateTokenizer(allow_logic_escapes),
                m_compiler(logic_compiler)
        {
        }

        void OnErrorUnbalancedEscapes(const size_t line_number) override
        {
            m_compiler.ReportError(MGF::TextTemplate_unbalanced_escapes_48101, static_cast<int>(line_number));
        }

        void OnErrorTokenNotEnded(const TextTemplateToken& token) override
        {
            m_compiler.ReportError(MGF::TextTemplate_end_reached_while_in_logic_or_fill_48102,
                                   ( token.type == TextTemplateToken::Type::Logic ) ? "logic" : "a fill",
                                   static_cast<int>(token.section_line_number_start));
        }

    private:
        LogicCompiler& m_compiler;
    };


    // maintain a mapping of the text template line numbers to the CSPro logic
    struct TextTemplateLineAdjuster : Logic::SourceBuffer::LineAdjuster
    {
        std::map<size_t, size_t> line_map;

        size_t GetLineNumber(const size_t line_number) override
        {
            for( size_t i = line_number; i > 0; --i )
            {
                if( line_map.count(i) != 0 )
                {
                    // only the first output line in each section is stored, so the exact line number has to be calculated
                    return line_map.find(i)->second + ( line_number - i );
                }
            }

            return 0;
        }
    };
}


std::unique_ptr<Logic::SourceBuffer> LogicCompiler::ConvertTextTemplateToSourceBuffer(const std::string_view text_template_sv, const bool allow_logic_escapes)
{
    EngineTextTemplateTokenizer text_template_tokenizer(*this, allow_logic_escapes);

    if( !text_template_tokenizer.Tokenize(text_template_sv, GetLogicSettings()) )
        return nullptr;

    // create the logic to run this text template
    std::string logic;

    auto text_template_line_adjuster = std::make_unique<TextTemplateLineAdjuster>();
    size_t source_line = 1;
    size_t output_line = 1;

    for( const TextTemplateToken& token : text_template_tokenizer.GetTokens() )
    {
        const size_t token_text_newlines = CountNewlines(token.text);

        text_template_line_adjuster->line_map.try_emplace(output_line, source_line);
        source_line += token_text_newlines;

        if( token.type == TextTemplateToken::Type::DirectText )
        {
            // the text template's direct text will be added to the string literal conserver
            logic.append(FormatText("$.write(%s := %d, %d);",
                                    WriteTypeNamedArgument,
                                    static_cast<int>(Nodes::TextTemplate::Type::DirectText),
                                    ConserveConstant(token.text)));

            ++output_line;
        }

        else if( token.type == TextTemplateToken::Type::DoubleTilde ||
                 token.type == TextTemplateToken::Type::TripleTilde )
        {
            logic.append(FormatText("$.write(%s := %d, %d, %s);",
                                    WriteTypeNamedArgument,
                                    static_cast<int>(Nodes::TextTemplate::Type::TextFill),
                                    ( token.type == TextTemplateToken::Type::DoubleTilde ) ? 1 : 0,
                                    token.text.c_str()));

            output_line += token_text_newlines + 1;
        }

        else
        {
            ASSERT(token.type == TextTemplateToken::Type::Logic);

            logic.append(token.text);

            output_line += token_text_newlines + 1;
        }

        logic.push_back('\n');
    }


    auto source_buffer = std::make_unique<Logic::SourceBuffer>(std::move(logic));
    source_buffer->SetLineAdjuster(std::move(text_template_line_adjuster));

    return source_buffer;
}
