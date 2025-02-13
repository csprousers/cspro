#include "stdafx.h"
#include "IncludesCC.h"
#include "Audio.h"


LogicAudio* LogicCompiler::CompileLogicAudioDeclaration()
{
    std::wstring audio_name = CompileNewSymbolName();

    auto logic_audio = std::make_shared<LogicAudio>(std::move(audio_name));

    m_engineData->AddSymbol(logic_audio);

    return logic_audio.get();
}


int LogicCompiler::CompileLogicAudioDeclarations()
{
    ASSERT(Tkn == TOKKWAUDIO);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicAudio* logic_audio = CompileLogicAudioDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_audio);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicAudioComputeInstruction(logic_audio);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicAudioComputeInstruction(const LogicAudio* logic_audio_from_declaration/* = nullptr*/)
{
    const Symbol* lhs_symbol = logic_audio_from_declaration;
    const int lhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Audio));

    if( lhs_symbol == nullptr )
    {
        ASSERT(Tkn == TOKAUDIO);
        lhs_symbol = &NPT_Ref(Tokstindex);
        ASSERT(lhs_symbol->IsA(SymbolType::Audio) || ( lhs_symbol->IsA(SymbolType::Item) && lhs_subscript_compilation != -1 ));

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Audio_invalid_assignment_47201);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();

    // an Audio can be assigned another Audio or a Document object
    if( Tkn != TOKAUDIO && Tkn != TOKDOCUMENT )
        IssueError(MGF::Audio_invalid_assignment_47201);

    auto& symbol_compute_with_subscript_node = CreateNode<Nodes::SymbolComputeWithSubscript>(FunctionCode::AUDIOFN_COMPUTE_CODE);

    symbol_compute_with_subscript_node.next_st = -1;
    symbol_compute_with_subscript_node.lhs_symbol_index = lhs_symbol->GetSymbolIndex();
    symbol_compute_with_subscript_node.lhs_subscript_compilation = lhs_subscript_compilation;
    symbol_compute_with_subscript_node.rhs_symbol_index = Tokstindex;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    NextToken();

    if( logic_audio_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_with_subscript_node);
}


int LogicCompiler::CompileLogicAudioFunctions()
{
    const FunctionCode function_code = CurrentToken.function_details->code;

    ASSERT(CurrentToken.symbol != nullptr && CurrentToken.symbol->IsOneOf(SymbolType::Audio, SymbolType::Item));
    const Symbol& symbol = *CurrentToken.symbol;

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_code, symbol, CurrentToken.symbol_subscript_compilation,
                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // audio_name.clear()
    // audio_name.length()
    // audio_name.stop()
    if( function_code == FunctionCode::AUDIOFN_CLEAR_CODE ||
        function_code == FunctionCode::AUDIOFN_LENGTH_CODE ||
        function_code == FunctionCode::AUDIOFN_STOP_CODE)
    {
        // no arguments
    }

    // audio_name.concat(audio|filename)
    else if( function_code == FunctionCode::AUDIOFN_CONCAT_CODE )
    {
        if( Tkn == TOKAUDIO )
        {
            symbol_va_with_subscript_node.arguments[0] = -1 * Tokstindex;
            symbol_va_with_subscript_node.arguments[1] = CurrentToken.symbol_subscript_compilation;

            NextToken();
        }

        else
        {
            symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
        }
    }

    // audio_name.load(filename)
    // audio_name.save(filename)
    else if( function_code == FunctionCode::AUDIOFN_LOAD_CODE ||
             function_code == FunctionCode::AUDIOFN_SAVE_CODE)
    {
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
    }

    // audio_name.play([message])
    // audio_name.recordInteractive([message])
    else if( function_code == FunctionCode::AUDIOFN_PLAY_CODE ||
             function_code == FunctionCode::AUDIOFN_RECORD_INTERACTIVE_CODE)
    {
        if( Tkn != TOKRPAREN )
            symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
    }

    // audio_name.record([seconds])
    else if( function_code == FunctionCode::AUDIOFN_RECORD_CODE )
    {
        if( Tkn != TOKRPAREN )
            symbol_va_with_subscript_node.arguments[0] = exprlog();
    }

    else
    {
        ASSERT(false);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
