#include "stdafx.h"
#include "IncludesCC.h"
#include "StringWriter.h"
#include "Nodes/Various.h"


StringWriter* LogicCompiler::CompileStringWriterDeclaration(const bool compiling_function_parameter)
{
    std::string string_writer_name = CompileNewSymbolName();
    Nodes::EncodeType encode_type = Nodes::EncodeType::Default;

    // read the optional encoding type
    if( NextKeywordIf(TOKLPAREN) )
    {
        if( compiling_function_parameter )
            IssueError(MGF::StringWriter_encoding_specified_for_func_param_100360);

        encode_type = static_cast<Nodes::EncodeType>(NextKeywordOrError(Nodes::EncodeTypeStrings));

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
    }

    auto string_writer = std::make_shared<StringWriter>(std::move(string_writer_name), encode_type);

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
