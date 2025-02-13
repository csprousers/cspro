#include "stdafx.h"
#include "IncludesCC.h"
#include "BinarySymbol.h"
#include "Nodes/Strings.h"
#include <engine/VarT.h>


int LogicCompiler::CompileStringExpression()
{
    // this method compiles a string expression, allowing the concatenation of strings using the + operator

    auto check_if_object_is_string = [&](const Symbol& symbol)
    {
        if( SymbolCalculator::GetDataType(symbol) != DataType::String )
            IssueError(MGF::object_of_type_expected_detailed_33117, symbol.GetName().c_str(), ToString(DataType::String));
    };

    auto compile_single_string_expression = [&]() -> int
    {
        // compile the string itself (which will be made negative if we need to process the string at runtime)
        int string_expression;

        switch( Tkn )
        {
            case TOKARRAY:
            {
                check_if_object_is_string(NPT_Ref(Tokstindex));
                string_expression = CompileLogicArrayReference();
                break;
            }

            case TOKHASHMAP:
            {
                check_if_object_is_string(NPT_Ref(Tokstindex));
                string_expression = CompileLogicHashMapReference();
                break;
            }

            case TOKFUNCTION:
            {
                if( !IsCurrentTokenString() )
                    IssueError(MGF::string_expression_invalid_96);

                string_expression = rutfunc_COMPILER_DLL_TODO();
                break;
            }

            case TOKLIST:
            {
                check_if_object_is_string(NPT_Ref(Tokstindex));
                string_expression = CompileLogicListReference();
                break;
            }

            case TOKSCTE:
            {
                string_expression = CreateStringLiteralNode(Tokstr);
                NextToken();
                break;
            }

            case TOKUSERFUNCTION:
            {
                check_if_object_is_string(NPT_Ref(Tokstindex));
                string_expression = rutfunc_COMPILER_DLL_TODO();
                break;
            }

            case TOKVAR:
            {
                check_if_object_is_string(NPT_Ref(Tokstindex));
                string_expression = -1 * varsanal_COMPILER_DLL_TODO(VPT(Tokstindex)->GetFmt());
                break;
            }

            case TOKWORKSTRING:
            {
                string_expression = CreateVariableArgumentsNode(FunctionCode::WORKSTRING_CODE, { Tokstindex });
                NextToken();
                break;
            }

            default:
            {
                IssueError(MGF::string_expression_invalid_96);
            }
        }

        // return the string expression immediately if there is no substring and it is a type
        // that has a runtime function that directly evaluations the object to a string
        bool specifying_substring = ( Tkn == TOKLBRACK );

        if( !specifying_substring && string_expression >= 0 )
            return string_expression;

        // otherwise create a node for evaluation and process any substring values
        auto& string_expression_node = CreateNode<Nodes::StringExpression>(FunctionCode::CHOBJ_CODE);

        string_expression_node.string_expression = string_expression;
        string_expression_node.substring_index_expression = -1;
        string_expression_node.substring_length_expression = -1;

        if( specifying_substring )
        {
            NextToken();

            string_expression_node.substring_index_expression = exprlog();

            if( Tkn == TOKCOLON )
            {
                NextToken();

                string_expression_node.substring_length_expression = exprlog();
            }

            IssueErrorOnTokenMismatch(TOKRBRACK, MGF::string_substring_invalid_95);

            NextToken();
        }

        return GetProgramIndex(string_expression_node);
    };


    // compile the first string
    int expression = compile_single_string_expression();

    // return the compilation of a single expression...
    if( Tkn != TOKADDOP )
    {
        return expression;
    }

    // ...or process the automatic concatenation of strings using the + operator
    else
    {
        std::vector<int> expressions = { expression };

        while( Tkn == TOKADDOP )
        {
            NextToken();
            expressions.emplace_back(compile_single_string_expression());
        }

        return CreateVariableArgumentsWithSizeNode(FunctionCode::FNCONCAT_CODE, expressions);
    }
}


int LogicCompiler::CompileStringExpressionWithStringLiteralCheck(const std::function<void(const std::wstring&)>& string_literal_check_callback)
{
    if( Tkn != TOKSCTE )
        return CompileStringExpression();

    std::wstring saved_token_text = Tokstr;
    const Logic::BasicToken* next_basic_token = PeekNextBasicToken();

    int expression = CompileStringExpression();

    // from CheckNextTokenHelper: prevent things like: "A" + "B"
    if( ( next_basic_token != nullptr ) &&
        ( next_basic_token->token_code == TOKCOMMA || next_basic_token->token_code == TOKRPAREN || next_basic_token->token_code == TOKSEMICOLON ) )
    {
        string_literal_check_callback(saved_token_text);
    }

    return expression;
}


int LogicCompiler::CompilePortableColorText()
{
    // compiles a string and, if a string literal, checks that it is a valid PortableColor
    return CompileStringExpressionWithStringLiteralCheck(
        [&](const std::wstring& text)
        {
            if( !PortableColor::FromString(text).has_value() )
                IssueError(MGF::color_invalid_2036, text.c_str());
        });
}


int LogicCompiler::CompileSymbolNameText(const SymbolType required_symbol_type/* = SymbolType::None*/, const bool throw_exception_is_symbol_is_not_found/* = true*/)
{
    // compiles a string and, if a string literal, checks that it is a valid symbol name
    return CompileStringExpressionWithStringLiteralCheck(
        [&](const std::wstring& text)
        {
            try
            {
                const Symbol& symbol = m_symbolTable.FindSymbolWithDotNotation(text, required_symbol_type);

                if( required_symbol_type != SymbolType::None && !symbol.IsA(required_symbol_type) )
                    IssueError(MGF::symbol_invalid_type_93008, text.c_str(), ToString(required_symbol_type));
            }

            catch( const Logic::SymbolTable::Exception& exception )
            {
                if( throw_exception_is_symbol_is_not_found )
                    IssueError(exception.GetCompilerErrorMessageNumber(), text.c_str());
            }
        });
}


int LogicCompiler::CompileFillText()
{
    auto& text_fill_node = CreateNode<Nodes::TextFill>();

    if( BinarySymbol::IsBinaryToken(Tkn) )
    {
        text_fill_node.data_type = DataType::Binary;
        text_fill_node.symbol_index_or_expression = Tokstindex;
        text_fill_node.subscript_compilation = CurrentToken.symbol_subscript_compilation;
        NextToken();
    }

    else
    {
        text_fill_node.data_type = GetCurrentTokenDataType();
        text_fill_node.symbol_index_or_expression = CompileExpression(text_fill_node.data_type);
    }

    return GetProgramIndex(text_fill_node);
}


int LogicCompiler::CompileStringComputeInstruction()
{
    const Symbol& symbol = NPT_Ref(Tokstindex);

    ASSERT(symbol.IsOneOf(SymbolType::Array,
                          SymbolType::UserFunction,
                          SymbolType::Variable,
                          SymbolType::WorkString));

    if( !IsString(symbol) )
    {
        ASSERT(false);
        IssueError(MGF::statement_invalid_1);
    }

    int program_index;

    // if not specifying a substring, we can optimize the code for strings
    if( symbol.IsA(SymbolType::WorkString) && IsNextToken(TOKEQOP) )
    {
        auto& symbol_reset_node = CreateNode<Nodes::SymbolReset>(FunctionCode::WORKSTRING_COMPUTE_CODE);

        symbol_reset_node.next_st = -1;
        symbol_reset_node.symbol_index = symbol.GetSymbolIndex();

        NextToken();

        NextToken();
        symbol_reset_node.initialize_value = CompileStringExpression();

        program_index = GetProgramIndex(symbol_reset_node);
    }

    // otherwise potentially process any substring values
    else
    {
        auto& string_compute_node = CreateNode<Nodes::StringCompute>(FunctionCode::STRING_COMPUTE_CODE);

        string_compute_node.next_st = -1;
        string_compute_node.symbol_value_node_index = CompileDestinationVariable(&symbol);
        string_compute_node.substring_index_expression = -1;
        string_compute_node.substring_length_expression = -1;

        if( Tkn == TOKLBRACK )
        {
            NextToken();
            string_compute_node.substring_index_expression = exprlog();

            if( Tkn == TOKCOLON )
            {
                NextToken();
                string_compute_node.substring_length_expression = exprlog();
            }

            IssueErrorOnTokenMismatch(TOKRBRACK, MGF::string_substring_invalid_95);

            NextToken();
        }

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

        NextToken();

        string_compute_node.string_expression = CompileStringExpression();

        program_index = GetProgramIndex(string_compute_node);
    }

    if( IsArithmeticOperator(Tkn) )
        IssueError(MGF::string_expression_invalid_96);

    return program_index;
}
