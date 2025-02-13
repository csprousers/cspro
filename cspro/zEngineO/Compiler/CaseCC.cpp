#include "stdafx.h"
#include "IncludesCC.h"
#include <engine/Dict.h>
#include <zCaseO/CaseAccess.h>


int LogicCompiler::CompileCaseFunctions()
{
    // compiling dictionary_name.view([viewer options]); // ENGINECR_TODO: move to Case/DataSource
    const Logic::FunctionDetails* function_details = CurrentToken.function_details;
    DICT& dictionary = *assert_cast<DICT*>(CurrentToken.symbol);

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_details->code,
                                                       dictionary, CurrentToken.symbol_subscript_compilation,
                                                       function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    ASSERT(function_details->code == FunctionCode::CASEFN_VIEW_CODE);
    dictionary.GetCaseAccess()->SetRequiresFullAccess();

    symbol_va_with_subscript_node.arguments[0] = CompileViewerOptions(true);

    if( symbol_va_with_subscript_node.arguments[0] == -1 )
        NextToken();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return WrapNodeAroundValidDataAccessCheck(GetProgramIndex(symbol_va_with_subscript_node),
                                              dictionary,
                                              function_details->return_data_type);
}
