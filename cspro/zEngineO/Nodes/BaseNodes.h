#pragma once

enum FunctionCode : int;
enum class SubscriptValueType : int { Implicit, ConstantInteger, Expression };


namespace Nodes
{
    struct ElementReference
    {
        FunctionCode function_code;
        int symbol_index;
        int element_expressions[1];
    };

    
    struct FunctionCall
    {
        FunctionCode function_code;
        int next_st;
        int expression;
    };


    struct ItemSubscript
    {
        enum class SubscriptType : int { ImplicitSinglyOccurring, ImplicitMustEvaluate, Specified };
        SubscriptType subscript_type;
        int subscript_arguments[1];
    };


    struct List
    {
        int number_elements;
        int elements[1];
    };


    struct Operator
    {
        int oper;
        int left_expr;
        int right_expr;
    };


    struct ScopeChange
    {
        FunctionCode function_code;
        int next_st;
        int program_index;
        int local_symbol_indices_list;
        int local_symbol_names_list;
    };


    struct Statement
    {
        FunctionCode function_code;
        int next_st;
    };


    struct SymbolCompute
    {
        FunctionCode function_code;
        int next_st;
        int lhs_symbol_index;
        SymbolType rhs_symbol_type;
        int rhs_symbol_index;
    };


    struct SymbolComputeWithSubscript
    {
        FunctionCode function_code;
        int next_st;
        int lhs_symbol_index;
        int lhs_subscript_compilation;
        int rhs_symbol_index;
        int rhs_subscript_compilation;
    };


    struct SymbolReset
    {
        FunctionCode function_code;
        int next_st;
        int symbol_index;
        int initialize_value;
    };


    struct SymbolValue
    {
        int symbol_index;
        int symbol_compilation;
    };


    struct SymbolVariableArguments
    {
        FunctionCode function_code;
        int symbol_index;
        int arguments[1];
    };


    struct SymbolVariableArgumentsWithSubscript
    {
        FunctionCode function_code;
        int symbol_index;
        int subscript_compilation;
        int arguments[1];
    };


    struct VariableArguments
    {
        FunctionCode function_code;
        int arguments[1];
    };


    struct VariableArgumentsWithSize
    {
        FunctionCode function_code;
        int number_arguments;
        int arguments[1];
    };
}


struct FNN_NODE
{
    int fn_code;
    int fn_nargs;
    int fn_expr[1];
};

static_assert(sizeof(Nodes::VariableArgumentsWithSize) == sizeof(FNN_NODE));


struct CONST_NODE
{
    int const_type;
    int const_index;
};


struct FNVARIOUS_NODE // COMPILER_DLL_TODO remove, use Nodes::VariableArguments instead
{
    int fn_code;
    int fn_expr[1];
};
