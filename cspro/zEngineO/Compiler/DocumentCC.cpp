#include "stdafx.h"
#include "IncludesCC.h"
#include "Document.h"


LogicDocument* LogicCompiler::CompileLogicDocumentDeclaration()
{
    std::wstring document_name = CompileNewSymbolName();

    auto logic_document = std::make_shared<LogicDocument>(std::move(document_name));

    m_engineData->AddSymbol(logic_document);

    return logic_document.get();
}


int LogicCompiler::CompileLogicDocumentDeclarations()
{
    ASSERT(Tkn == TOKKWDOCUMENT);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicDocument* logic_document = CompileLogicDocumentDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_document);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicDocumentComputeInstruction(logic_document);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicDocumentComputeInstruction(const LogicDocument* logic_document_from_declaration/* = nullptr*/)
{
    const Symbol* lhs_symbol = logic_document_from_declaration;
    const int lhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Document));

    if( lhs_symbol == nullptr )
    {
        ASSERT(Tkn == TOKDOCUMENT);
        lhs_symbol = &NPT_Ref(Tokstindex);
        ASSERT(lhs_symbol->IsA(SymbolType::Document) || ( lhs_symbol->IsA(SymbolType::Item) && lhs_subscript_compilation != -1 ));

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Document_invalid_assignment_47221);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();

    auto& symbol_compute_with_subscript_node = CreateNode<Nodes::SymbolComputeWithSubscript>(FunctionCode::DOCUMENTFN_COMPUTE_CODE);

    symbol_compute_with_subscript_node.next_st = -1;
    symbol_compute_with_subscript_node.lhs_symbol_index = lhs_symbol->GetSymbolIndex();
    symbol_compute_with_subscript_node.lhs_subscript_compilation = lhs_subscript_compilation;

    // a Document can be assigned another Document or an Audio/Geometry/Image object
    if( Tkn == TOKDOCUMENT || Tkn == TOKAUDIO || Tkn == TOKGEOMETRY || Tkn == TOKIMAGE )
    {
        symbol_compute_with_subscript_node.rhs_symbol_index = Tokstindex;
        symbol_compute_with_subscript_node.rhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

        NextToken();
    }

    // or it can assigned a string
    else if( IsCurrentTokenString() )
    {
        symbol_compute_with_subscript_node.rhs_symbol_index = -1;
        symbol_compute_with_subscript_node.rhs_subscript_compilation = CompileStringExpression();
    }

    else
    {
        IssueError(MGF::Document_invalid_assignment_47221);
    }

    if( logic_document_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_with_subscript_node);
}


int LogicCompiler::CompileLogicDocumentFunctions()
{
    // compiling document_name.clear();
    //           document_name.load(filename);
    //           document_name.save(filename);
    //           document_name.view([viewer options]);
    const FunctionCode function_code = CurrentToken.function_details->code;

    ASSERT(CurrentToken.symbol != nullptr && CurrentToken.symbol->IsOneOf(SymbolType::Document, SymbolType::Item));
    const Symbol& symbol = *CurrentToken.symbol;

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_code, symbol, CurrentToken.symbol_subscript_compilation,
                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    if( function_code != FunctionCode::DOCUMENTFN_VIEW_CODE )
        NextToken();

    // load and save take a string argument
    if( function_code == FunctionCode::DOCUMENTFN_LOAD_CODE || function_code == FunctionCode::DOCUMENTFN_SAVE_CODE )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
    }

    // view can take optional viewer options
    else if( function_code == FunctionCode::DOCUMENTFN_VIEW_CODE )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileViewerOptions(true);

        if( symbol_va_with_subscript_node.arguments[0] == -1 )
            NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
