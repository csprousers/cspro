#include "stdafx.h"
#include "IncludesCC.h"
#include "StringWriter.h"
#include "Nodes/TextTemplate.h"


StringWriter* LogicCompiler::CompileStringWriterDeclaration(const bool compiling_function_parameter)
{
    std::string string_writer_name = CompileNewSymbolName();
    std::variant<EncodeType, std::reference_wrapper<const Symbol>> encode_type_or_symbol = EncodeType::Default;

    // the StringWriter can optionally be declared with a specified encoding type or as based on a Report
    if( NextKeywordIf(TOKLPAREN) )
    {
        if( compiling_function_parameter )
            IssueError(MGF::StringWriter_option_invalid_for_function_parameter_100360);

        const size_t encode_type = NextKeyword(EncodeTypeStrings);

        if( encode_type != 0 )
        {
            encode_type_or_symbol = static_cast<EncodeType>(encode_type);
        }

        else
        {
            NextToken();

            if( Tkn == TOKREPORT )
            {
                encode_type_or_symbol = NPT_Ref(Tokstindex);
            }

            else
            {
                // NextKeywordOrError will display the valid options
                NextKeywordOrError(EncodeTypeStrings);
                ASSERT(false);
            }
        }

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
    }

    auto string_writer = std::holds_alternative<EncodeType>(encode_type_or_symbol) ?
        std::make_shared<StringWriter>(std::move(string_writer_name), std::get<0>(encode_type_or_symbol)) :
        std::make_shared<StringWriter>(std::move(string_writer_name), std::get<1>(encode_type_or_symbol).get());

    m_engineData->AddSymbol(string_writer);

    return string_writer.get();
}


int LogicCompiler::CompileStringWriterDeclarations()
{
    ASSERT(Tkn == TOKKWSTRINGWRITER);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        const StringWriter* const string_writer = CompileStringWriterDeclaration(false);

        AddSymbolResetNode(symbol_reset_node, *string_writer);

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileStringWriterFunctions()
{
    // compiling: string_writer.toString();
    const FunctionCode function_code = CurrentToken.function_details->code;
    const StringWriter& string_writer = *assert_cast<const StringWriter*>(CurrentToken.symbol);

    CheckTextTemplateIsCurrentlyAccessible(string_writer);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();
    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    Nodes::SymbolVariableArguments& symbol_va_node = CreateSymbolVariableArgumentsNode(function_code, string_writer, 0);
    return GetProgramIndex(symbol_va_node);
}
