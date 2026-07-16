#include "stdafx.h"
#include "IncludesCC.h"


bool LogicCompiler::IsNumericConstantInteger(double value)
{
    return ( floorl(value) == value );
}


bool LogicCompiler::IsNumericConstantInteger() const
{
    ASSERT(Tkn == TOKCTE);
    return IsNumericConstantInteger(Tokvalue);
}


int LogicCompiler::CompileNumericComputeInstruction()
{
    const Symbol& symbol = NPT_Ref(Tokstindex);
    ASSERT(IsNumeric(symbol));

    auto& symbol_compute_expression_node = CreateNode<Nodes::SymbolComputeExpression>();
    symbol_compute_expression_node.next_st = -1;

    switch( symbol.GetType() )
    {
        case SymbolType::Array:
            symbol_compute_expression_node.function_code = FunctionCode::CPT_CODE;
            symbol_compute_expression_node.lhs_symbol_index_or_compilation = CompileLogicArrayReference();
            break;

        case SymbolType::WorkVariable:
            NextToken();
            symbol_compute_expression_node.function_code = FunctionCode::WORKVARIABLE_COMPUTE_CODE;
            symbol_compute_expression_node.lhs_symbol_index_or_compilation = symbol.GetSymbolIndex();
            break;

        default:
            throw ProgrammingErrorException();
    }

    IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

    NextToken();
    symbol_compute_expression_node.rhs_expression = exprlog();

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_expression_node);
}
