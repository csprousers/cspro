#include "stdafx.h"
#include "IncludesCC.h"
#include "CompilationExtendedInformation.h"
#include "ValueSet.h"
#include "WorkString.h"
#include "WorkVariable.h"
#include "Nodes/Switch.h"


int LogicCompiler::CompileSwitch(bool compiling_when, const std::function<std::vector<int>()>& result_destinations_compiler,
                                 const std::function<int(size_t)>& action_compiler)
{
    const FunctionCode function_code = compiling_when ?  WHEN_CODE :  RECODE_CODE;
    const TokenCode end_token_code =   compiling_when ? TOKENDWHEN : TOKENDRECODE;
    const int error_message_number =   compiling_when ?         56 :           57;

    // read the condition variables
    std::vector<std::tuple<DataType, int>> condition_values;

    while( true )
    {
        const DataType value_data_type = GetCurrentTokenDataType();
        condition_values.emplace_back(value_data_type, CompileExpression(value_data_type));

        if( Tkn != TOKDOUBLECOLON )
            break;

        NextToken();
    }

    const std::vector<int> result_destinations = result_destinations_compiler();

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    NextToken();

    // process each condition
    std::vector<std::tuple<TokenCode, int>> condition_checks;
    std::vector<int> actions;

    while( Tkn != end_token_code )
    {
        for( size_t i = 0; i < condition_values.size(); ++i )
        {
            const DataType data_type = std::get<0>(condition_values[i]);

            if( i > 0 )
            {
                IssueErrorOnTokenMismatch(TOKDOUBLECOLON, error_message_number);
                NextToken();
            }

            // all conditions (or the last condition) can be left blank
            if( Tkn == TOKARROW && ( i == 0 || i == ( condition_values.size() - 1 ) ) )
            {
                for( ; i < condition_values.size(); ++i )
                    condition_checks.emplace_back(TokenCode::Unspecified, -1);
            }

            // a single condition can be left blank
            else if( Tkn == TOKDOUBLECOLON )
            {
                condition_checks.emplace_back(TokenCode::Unspecified, -1);
            }

            // relational operators can be used
            else if( IsRelationalOperator(Tkn) )
            {
                TokenCode token_code = Tkn;
                NextToken();
                condition_checks.emplace_back(token_code, CompileExpression(data_type));
            }

            // otherwise compile an in expression
            else
            {
                condition_checks.emplace_back(TokenCode::TOKIN, CompileInNodes(data_type));
            }
        }

        IssueErrorOnTokenMismatch(TOKARROW, error_message_number);

        NextToken();

        for( size_t i = 0; i < result_destinations.size(); ++i )
            actions.emplace_back(action_compiler(i));
    }

    IssueErrorOnTokenMismatch(end_token_code, error_message_number);

    NextToken();

    ASSERT(condition_checks.size() % condition_values.size() == 0);
    ASSERT(actions.size() % result_destinations.size() == 0);
    ASSERT(condition_checks.size() / condition_values.size() == ( actions.size() / result_destinations.size() ));

    const int ints_needed_for_compilation = condition_values.size() * 2 + result_destinations.size() + condition_checks.size() * 2  + actions.size();

    auto& switch_node = CreateVariableSizeNode<Nodes::Switch>(function_code, ints_needed_for_compilation);

    switch_node.number_condition_values = condition_values.size();
    switch_node.number_destinations = result_destinations.size();
    switch_node.number_actions = actions.size() / result_destinations.size();

    int* compiled_code = switch_node.expressions;

    for( const auto& [data_type, expression] : condition_values )
    {
        *(compiled_code++) = static_cast<int>(data_type);
        *(compiled_code++) = expression;
    }

    memcpy(compiled_code, result_destinations.data(), sizeof(result_destinations[0]) * result_destinations.size());
    compiled_code += result_destinations.size();

    for( const auto& [token_code, expression] : condition_checks )
    {
        *(compiled_code++) = static_cast<int>(token_code);
        *(compiled_code++) = expression;
    }

    memcpy(compiled_code, actions.data(), sizeof(actions[0]) * actions.size());

    return GetProgramIndex(switch_node);
}


int LogicCompiler::CompileWhen()
{
    std::function<std::vector<int>()> result_destinations_compiler = []
    {
        return std::vector({ -1 });
    };

    std::function<int(size_t)> action_compiler = [&](size_t/* index*/)
    {
        // allow empty actions
        if( Tkn == TOKSEMICOLON )
        {
            NextToken();
            return -1;
        }

        else
        {
            return instruc_COMPILER_DLL_TODO(false, false);
        }
    };

    return CompileSwitch(true, result_destinations_compiler, action_compiler);
}


int LogicCompiler::CompileRecode()
{
    // first see if they are using the pre-7.4 version of recode
    bool using_old_recode = SO::EqualsNoCase(Tokstr, _T("box"));

    if( !using_old_recode )
    {
        MarkInputBufferToRestartLater();

        while( !IsNextToken({ TOKARROW, TOKSEMICOLON, TOKEOP }) )
        {
            NextToken();

            if( Tkn == TOKIMPLOP )
            {
                using_old_recode = true;
                break;
            }
        }

        RestartFromMarkedInputBuffer();
    }

    if( using_old_recode )
        IssueError(MGF::deprecation_removed_old_recode_95022);

    NextToken();

    // the new, 7.4+, version of recode
    std::vector<DataType> destination_data_types;

    std::function<std::vector<int>()> result_destinations_compiler =
        [&]()
        {
            IssueErrorOnTokenMismatch(TOKARROW, MGF::switch_expecting_array_57);

            // compile the destination variables
            std::vector<int> result_destinations;

            while( true )
            {
                NextToken();

                Symbol* symbol = nullptr;

                // allow the creation of numeric/string variables on the fly
                if( bool numeric = ( Tkn == TOKNUMERIC ); numeric || Tkn == TOKALPHA || Tkn == TOKSTRING )
                {
                    if( numeric )
                    {
                        symbol = CompileWorkVariableDeclaration();
                    }

                    else
                    {
                        symbol = CompileLogicStringDeclaration(Tkn);
                    }

                    destination_data_types.emplace_back(SymbolCalculator::GetDataType(*symbol));
                    result_destinations.emplace_back(CompileDestinationVariable(symbol));
                }

                else
                {
                    DataType destination_data_type = GetCurrentTokenDataType();
                    destination_data_types.emplace_back(destination_data_type);
                    result_destinations.emplace_back(CompileDestinationVariable(destination_data_type));
                }

                if( Tkn != TOKDOUBLECOLON )
                    break;
            }

            return result_destinations;
        };

    std::function<int(size_t)> action_compiler =
        [&](size_t index)
        {
            int action = CompileExpression(destination_data_types[index]);

            if( ( index + 1 ) == destination_data_types.size() )
            {
                IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);
            }

            else
            {
                IssueErrorOnTokenMismatch(TOKDOUBLECOLON, MGF::switch_expecting_double_colon_58);
            }

            NextToken();

            return action;
        };

    return CompileSwitch(false, result_destinations_compiler, action_compiler);
}


int LogicCompiler::CreateInNode(DataType data_type, int left_expr, int right_expr)
{
    auto& in_node = CreateNode<Nodes::In>(FunctionCode::IN_CODE);

    in_node.data_type = data_type;
    in_node.left_expr = left_expr;
    in_node.right_expr= right_expr;

    return GetProgramIndex(in_node);
}


int LogicCompiler::CompileInNodes(DataType data_type, CompilationExtendedInformation::InCrosstabInformation* in_crosstab_information/* = nullptr*/)
{
    ASSERT(data_type == DataType::Numeric || data_type == DataType::String);

    const Nodes::In::Entry* first_in_entry_node = nullptr;
    Nodes::In::Entry* previous_in_entry_node = nullptr;

    // allow a negative sign at the beginning of an in list
    if( Tkn == TOKMINOP )
        Tkn = TOKMINUS;

    while( true )
    {
        enum class ExpressionRead { Object, Special, Other };

        auto read_expression = [&](int& expression, bool low_value)
        {
            const Logic::BasicToken* next_basic_token = PeekNextBasicToken();

            // only constants are allowed for crosstabs
            if( in_crosstab_information != nullptr )
            {
                // note: this crosstab compilation will not work properly with negative numbers,
                // but it seems like the pre-8.0 compilation did not allow this either
                ASSERT(data_type == DataType::Numeric);

                const double value = Tokvalue;
                expression = CreateNumericConstantNode(value);

                NextToken();

                if( low_value )
                {
                    in_crosstab_information->ranges.emplace_back(value, value);
                    in_crosstab_information->implicit_highs.emplace_back(true);
                }

                else
                {
                    in_crosstab_information->ranges.back().high = value;
                    in_crosstab_information->implicit_highs.back() = false;
                }

                return ExpressionRead::Other;
            }

            // allow value sets
            else if( low_value && Tkn == TOKVALUESET )
            {
                if( GetSymbolValueSet(Tokstindex).GetDataType() != data_type )
                    IssueError(MGF::ValueSet_not_correct_data_type_941, ToString(data_type));

                expression = -1 * Tokstindex;

                NextToken();

                return ExpressionRead::Object;
            }

            // allow lists
            else if( low_value && Tkn == TOKLIST && next_basic_token != nullptr && next_basic_token->token_code != TokenCode::TOKLPAREN )
            {
                if( GetSymbolLogicList(Tokstindex).GetDataType() != data_type )
                    IssueError(MGF::List_not_correct_data_type_961, ToString(data_type));

                expression = -1 * Tokstindex;

                NextToken();

                return ExpressionRead::Object;
            }

            else if( data_type == DataType::Numeric )
            {
                // allow "special" as an alias for all special values
                if( low_value && Tkn == TOKFUNCTION && CurrentToken.function_details->code == FNSPECIAL_CODE )
                {
                    if( next_basic_token != nullptr && next_basic_token->token_code != TokenCode::TOKLPAREN )
                    {
                        NextToken();

                        return ExpressionRead::Special;
                    }
                }

                expression = factlog();

                return ExpressionRead::Other;
            }

            else
            {
                ASSERT(data_type == DataType::String);
                expression = CompileStringExpression();

                return ExpressionRead::Other;
            }
        };


        // create the compilation node and link the previous one (if applicable)
        auto& in_entry_node = CreateNode<Nodes::In::Entry>();

        if( first_in_entry_node == nullptr )
            first_in_entry_node = &in_entry_node;

        if( previous_in_entry_node != nullptr )
            previous_in_entry_node->next_entry_index = GetProgramIndex(in_entry_node);

        previous_in_entry_node = &in_entry_node;

        // read the low value
        const ExpressionRead expression_read = read_expression(in_entry_node.expression_low, true);

        // both values can be set at once for "special"
        if( expression_read == ExpressionRead::Special )
        {
            in_entry_node.expression_low = CreateNumericConstantNode(SpecialValues::SmallestSpecialValue());
            in_entry_node.expression_high = CreateNumericConstantNode(SpecialValues::LargestSpecialValue());
        }

        // objects do not allow high values
        else if( expression_read == ExpressionRead::Object )
        {
            in_entry_node.expression_high = -1;
        }

        // otherwise check for a high value
        else
        {
            ASSERT(expression_read == ExpressionRead::Other);

            if( Tkn == TOKCOLON )
            {
                NextToken();
                read_expression(in_entry_node.expression_high, false);
            }

            else
            {
                in_entry_node.expression_high = -1;
            }
        }

        // potentially read more nodes
        if( Tkn == TOKCOMMA )
        {
            NextToken();
        }

        // or end the last node
        else
        {
            previous_in_entry_node->next_entry_index= -1;

            return GetProgramIndex(*first_in_entry_node);
        }
    }
}
