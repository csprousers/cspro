#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "EngineFloatingPointMath.h"
#include "EngineStringComparer.h"
#include <zEngineO/Nodes/Switch.h>


std::optional<std::tuple<const int*, const int*>> CIntDriver::EvaluateSwitchConditions(int iExpr)
{
    const auto& switch_node = GetNode<Nodes::Switch>(iExpr);
    const int* condition_values = switch_node.expressions;
    const int* result_destinations = condition_values + ( switch_node.number_condition_values * 2 );
    const int* condition_checks = result_destinations + switch_node.number_destinations;
    const int* actions = condition_checks + ( switch_node.number_actions * ( switch_node.number_condition_values * 2 ) );

    // calculate the condition values
    std::vector<double> numeric_values;
    std::vector<std::wstring> string_values;
    std::vector<size_t> value_indices;

    for( int i = 0; i < switch_node.number_condition_values; ++i )
    {
        const DataType data_type = static_cast<DataType>(condition_values[i * 2]);
        int value_expression = condition_values[i * 2 + 1];

        if( IsNumeric(data_type) )
        {
            value_indices.emplace_back(numeric_values.size());
            numeric_values.emplace_back(evalexpr(value_expression));
        }

        else
        {
            ASSERT(IsString(data_type));

            value_indices.emplace_back(string_values.size());
            string_values.emplace_back(EvalAlphaExpr(value_expression));
        }
    }

    // check each condition
    for( int action_index = 0; action_index < switch_node.number_actions; ++action_index )
    {
        size_t condition_checks_index = action_index * ( switch_node.number_condition_values * 2 );
        bool conditions_match = true;

        for( int i = 0; conditions_match && i < switch_node.number_condition_values; ++i )
        {
            const TokenCode token_code = static_cast<TokenCode>(condition_checks[condition_checks_index++]);
            const int expression = condition_checks[condition_checks_index++];

            if( token_code == TokenCode::Unspecified )
                continue;

            const DataType data_type = static_cast<DataType>(condition_values[i * 2]);

            // numeric conditions
            if( IsNumeric(data_type) )
            {
                const double lhs_value = numeric_values[value_indices[i]];

                if( token_code == TokenCode::TOKIN )
                {
                    conditions_match = InWorker(expression, lhs_value);
                }

                else
                {
                    const double rhs_value = evalexpr(expression);

                    if( token_code == TokenCode::TOKEQOP )
                    {
                        conditions_match = FloatingPointMath::Equals(lhs_value, rhs_value);
                    }

                    else if( token_code == TokenCode::TOKNEOP )
                    {
                        conditions_match = !FloatingPointMath::Equals(lhs_value, rhs_value);
                    }

                    else if( token_code == TokenCode::TOKLTOP )
                    {
                        conditions_match = EngineFloatingPointMath::comparison<TokenCode::TOKLTOP>(lhs_value, rhs_value);
                    }

                    else if( token_code == TokenCode::TOKLEOP )
                    {
                        conditions_match = EngineFloatingPointMath::comparison<TokenCode::TOKLEOP>(lhs_value, rhs_value);
                    }

                    else if( token_code == TokenCode::TOKGEOP )
                    {
                        conditions_match = EngineFloatingPointMath::comparison<TokenCode::TOKGEOP>(lhs_value, rhs_value);
                    }

                    else if( token_code == TokenCode::TOKGTOP )
                    {
                        conditions_match = EngineFloatingPointMath::comparison<TokenCode::TOKGTOP>(lhs_value, rhs_value);
                    }

                    else
                    {
                        ASSERT(false);
                    }
                }
            }

            // string conditions
            else
            {
                ASSERT(IsString(data_type));

                const std::wstring& lhs_value = string_values[value_indices[i]];

                if( token_code == TokenCode::TOKIN )
                {
                    conditions_match = InWorker(expression, lhs_value);
                }

                else
                {
                    ASSERT(token_code == TokenCode::TOKEQOP ||
                           token_code == TokenCode::TOKNEOP ||
                           token_code == TokenCode::TOKLTOP ||
                           token_code == TokenCode::TOKLEOP ||
                           token_code == TokenCode::TOKGEOP ||
                           token_code == TokenCode::TOKGTOP);

                    const std::wstring rhs_value = EvalAlphaExpr(expression);
                    conditions_match = m_usingLogicSettingsV0 ? EngineStringComparer::V0::Evaluate(lhs_value, rhs_value, token_code) :
                                                                EngineStringComparer::V8::Evaluate(lhs_value, rhs_value, token_code);
                }
            }
        }

        // if everything matched, run the action
        if( conditions_match )
            return std::make_tuple(result_destinations, actions + ( action_index * switch_node.number_destinations ));
    }

    return std::nullopt;
}


double CIntDriver::exwhen(int iExpr)
{
    std::optional<std::tuple<const int*, const int*>> destinations_and_actions = EvaluateSwitchConditions(iExpr);

    if( destinations_and_actions.has_value() )
    {
        const int* action = std::get<1>(*destinations_and_actions);
        ExecuteProgramStatements(*action);
    }

    return 0;
}


double CIntDriver::exrecode(int iExpr)
{
    std::optional<std::tuple<const int*, const int*>> destinations_and_actions = EvaluateSwitchConditions(iExpr);

    if( destinations_and_actions.has_value() )
    {
        const auto& switch_node = GetNode<Nodes::Switch>(iExpr);
        const int* result_destination = std::get<0>(*destinations_and_actions);
        const int* action = std::get<1>(*destinations_and_actions);

        for( int i = 0; i < switch_node.number_destinations; ++i, ++result_destination, ++action )
        {
            const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(*result_destination);
            DataType data_type = SymbolCalculator::GetDataType(NPT_Ref(symbol_value_node.symbol_index));

            if( IsNumeric(data_type) )
            {
                AssignValueToSymbol(symbol_value_node, evalexpr(*action));
            }

            else if( IsString(data_type) )
            {
                AssignValueToSymbol(symbol_value_node, EvalAlphaExpr(*action));
            }

            else
            {
                throw ProgrammingErrorException();
            }
        }
    }

    return 0;
}
