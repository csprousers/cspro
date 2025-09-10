#include "stdafx.h"
#include "IncludesCC.h"
#include "Video.h"


LogicVideo* LogicCompiler::CompileLogicVideoDeclaration()
{
    std::string video_name = CompileNewSymbolName();

    auto logic_video = std::make_shared<LogicVideo>(std::move(video_name));

    m_engineData->AddSymbol(logic_video);

    return logic_video.get();
}


int LogicCompiler::CompileLogicVideoDeclarations()
{
    ASSERT(Tkn == TOKKWVIDEO);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicVideo* const logic_video = CompileLogicVideoDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_video);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicVideoComputeInstruction(logic_video);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicVideoComputeInstruction(const LogicVideo* const logic_video_from_declaration/* = nullptr*/)
{
    const Symbol* lhs_symbol = logic_video_from_declaration;
    const int lhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Video));

    if( lhs_symbol == nullptr )
    {
        ASSERT(Tkn == TOKVIDEO);
        lhs_symbol = &NPT_Ref(Tokstindex);
        ASSERT(lhs_symbol->IsA(SymbolType::Video) || ( lhs_symbol->IsA(SymbolType::Item) && lhs_subscript_compilation != -1 ));

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Video_invalid_assignment_48151);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();

    // a Video can be assigned another Video object
    if( Tkn != TOKVIDEO )
        IssueError(MGF::Video_invalid_assignment_48151);

    auto& symbol_compute_with_subscript_node = CreateNode<Nodes::SymbolComputeWithSubscript>(FunctionCode::VIDEOFN_COMPUTE_CODE);

    symbol_compute_with_subscript_node.next_st = -1;
    symbol_compute_with_subscript_node.lhs_symbol_index = lhs_symbol->GetSymbolIndex();
    symbol_compute_with_subscript_node.lhs_subscript_compilation = lhs_subscript_compilation;
    symbol_compute_with_subscript_node.rhs_symbol_index = Tokstindex;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    NextToken();

    if( logic_video_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_with_subscript_node);
}


int LogicCompiler::CompileLogicVideoFunctions()
{
    const FunctionCode function_code = CurrentToken.function_details->code;

    ASSERT(CurrentToken.symbol != nullptr && CurrentToken.symbol->IsOneOf(SymbolType::Video, SymbolType::Item));
    const Symbol& symbol = *CurrentToken.symbol;

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_code, symbol, CurrentToken.symbol_subscript_compilation,
                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // video_name.clear()
    if( function_code == FunctionCode::VIDEOFN_CLEAR_CODE )
    {
        // no arguments
    }

    // video_name.load(filename)
    // video_name.save(filename)
    else if( function_code == FunctionCode::VIDEOFN_LOAD_CODE ||
             function_code == FunctionCode::VIDEOFN_SAVE_CODE )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
    }

    else
    {
        ASSERT(false);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
