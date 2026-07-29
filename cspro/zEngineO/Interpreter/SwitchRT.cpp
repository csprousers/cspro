#include "stdafx.h"
#include "IncludesRT.h"
#include "List.h"
#include "StringComparer.h"
#include "ValueSet.h"
#include <zEngineO/Nodes/Switch.h>
#include <zToolsO/FloatingPointMath.h>
#include <zUtilO/Randomizer.h>
#include <zDictO/ValueProcessor.h>


std::optional<std::tuple<const int*, const int*>> LogicInterpreter::EvaluateSwitchConditions(const int program_index)
{
    const auto& switch_node = GetNode<Nodes::Switch>(program_index);
    const int* const condition_values = switch_node.expressions;
    const int* const result_destinations = condition_values + ( switch_node.number_condition_values * 2 );
    const int* const condition_checks = result_destinations + switch_node.number_destinations;
    const int* const actions = condition_checks + ( switch_node.number_actions * ( switch_node.number_condition_values * 2 ) );

    // calculate the condition values
    std::vector<double> numeric_values;
    std::vector<SharableString> string_values;
    std::vector<size_t> value_indices;

    for( int i = 0; i < switch_node.number_condition_values; ++i )
    {
        const DataType data_type = static_cast<DataType>(condition_values[i * 2]);
        const int value_expression = condition_values[i * 2 + 1];

        if( IsNumeric(data_type) )
        {
            value_indices.emplace_back(numeric_values.size());
            numeric_values.emplace_back(Evaluate<double>(value_expression));
        }

        else
        {
            ASSERT(IsString(data_type));

            value_indices.emplace_back(string_values.size());
            string_values.emplace_back(EvaluateSharableString(value_expression));
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
                    const double rhs_value = Evaluate<double>(expression);
                    conditions_match = FloatingPointMath::Evaluate(token_code, lhs_value, rhs_value);
                }
            }

            // string conditions
            else
            {
                ASSERT(IsString(data_type));

                const SharableString& lhs_value = string_values[value_indices[i]];

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

                    const SharableString rhs_value = EvaluateSharableString(expression);
                    conditions_match = m_usingLogicSettingsV0 ? EngineStringComparer::V0::Evaluate(*lhs_value, *rhs_value, token_code) :
                                                                EngineStringComparer::V8::Evaluate(*lhs_value, *rhs_value, token_code);
                }
            }
        }

        // if everything matched, run the action
        if( conditions_match )
            return std::make_tuple(result_destinations, actions + ( action_index * switch_node.number_destinations ));
    }

    return std::nullopt;
}


double LogicInterpreter::ex_when(const int program_index)
{
    const std::optional<std::tuple<const int*, const int*>> destinations_and_actions = EvaluateSwitchConditions(program_index);

    if( destinations_and_actions.has_value() )
    {
        const int* const action = std::get<1>(*destinations_and_actions);
        ExecuteProgramStatements(*action);
    }

    return 0;
}


double LogicInterpreter::ex_recode(const int program_index)
{
    const std::optional<std::tuple<const int*, const int*>> destinations_and_actions = EvaluateSwitchConditions(program_index);

    if( destinations_and_actions.has_value() )
    {
        const auto& switch_node = GetNode<Nodes::Switch>(program_index);
        const int* result_destination = std::get<0>(*destinations_and_actions);
        const int* action = std::get<1>(*destinations_and_actions);

        for( int i = 0; i < switch_node.number_destinations; ++i, ++result_destination, ++action )
        {
            const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(*result_destination);
            const DataType data_type = SymbolCalculator::GetDataType(NPT_Ref(symbol_value_node.symbol_index));

            if( IsNumeric(data_type) )
            {
                AssignValueToSymbol(symbol_value_node, Evaluate<double>(*action));
            }

            else if( IsString(data_type) )
            {
                AssignValueToSymbol(symbol_value_node, EvaluateSharableString(*action));
            }

            else
            {
                throw ProgrammingErrorException();
            }
        }
    }

    return 0;
}


bool LogicInterpreter::InWorker(const int in_node_expression, const std::variant<double, SharableString>& value,
                                const std::function<const std::variant<double, SharableString>&(int)>* const expression_evaluator/* = nullptr*/)
{
    const Nodes::In::Entry* in_node_entry = &GetNode<Nodes::In::Entry>(in_node_expression);
    const bool is_numeric = std::holds_alternative<double>(value);

    while( in_node_entry != nullptr )
    {
        bool in_range = false;

        // using a list or a value set
        if( in_node_entry->expression_low < 0 )
        {
            const Symbol& symbol = NPT_Ref(-1 * in_node_entry->expression_low);

            if( symbol.IsA(SymbolType::List) )
            {
                const LogicList& logic_list = assert_cast<const LogicList&>(symbol);

                in_range = is_numeric ? logic_list.Contains(std::get<double>(value)) :
                                        logic_list.Contains(std::get<SharableString>(value));
            }

            else
            {
                const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);
                const ValueProcessor& value_processor = value_set.GetValueProcessor();

                in_range = is_numeric ? value_processor.IsValid(std::get<double>(value)) :
                                        value_processor.IsValid(*std::get<SharableString>(value));
            }
        }

        // or a range
        else
        {
            const bool range_has_two_values = ( in_node_entry->expression_high != -1 );

            // numeric
            if( is_numeric )
            {
                auto get_number = [&](const int expression) -> double
                {
                    return ( expression_evaluator == nullptr ) ? Evaluate<double>(expression) :
                                                                 std::get<double>((*expression_evaluator)(expression));
                };

                const double low_value = get_number(in_node_entry->expression_low);

                if( range_has_two_values )
                {
                    const double high_value = get_number(in_node_entry->expression_high);

                    if( !IsSpecial(std::get<double>(value)) )
                    {
                        in_range = ( std::get<double>(value) >= low_value &&
                                     std::get<double>(value) <= high_value );
                    }

                    // handle the "special" alias that gets compiled as a range
                    else if( low_value == SpecialValues::SmallestSpecialValue() &&
                             high_value == SpecialValues::LargestSpecialValue() )
                    {
                        in_range = true;
                    }
                }

                else
                {
                    in_range = ( std::get<double>(value) == low_value );
                }
            }

            // alpha
            else
            {
                auto get_string_comparison = [&](const int expression)
                {
                    const SharableString rhs = ( expression_evaluator == nullptr ) ? EvaluateSharableString(expression) :
                                                                                     std::get<SharableString>((*expression_evaluator)(expression));

                    return m_usingLogicSettingsV0 ? EngineStringComparer::V0::Compare(std::get<SharableString>(value).GetString(), rhs.GetString()) :
                                                    std::get<SharableString>(value)->compare(*rhs);
                };

                const int alpha_comparison = get_string_comparison(in_node_entry->expression_low);
                in_range = ( alpha_comparison == 0 );

                if( !in_range && range_has_two_values && alpha_comparison > 0 )
                    in_range = ( get_string_comparison(in_node_entry->expression_high) <= 0 );
            }
        }

        if( in_range )
            return true;

        in_node_entry = ( in_node_entry->next_entry_index != -1 ) ? &GetNode<Nodes::In::Entry>(in_node_entry->next_entry_index) :
                                                                    nullptr;
    }

    return false;
}


double LogicInterpreter::ex_in(const int program_index)
{
    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        const auto& in_node = GetNode<Nodes::In>(program_index);
        return InWorker(in_node.right_expr, EvaluateVariant(in_node.data_type, in_node.left_expr));
    }

    else
    {
        const auto& operator_node = GetNode<Nodes::Operator>(program_index);
        const DataType data_type = ( GetNode<Nodes::Operator>(operator_node.left_expr).oper == CHOBJ_CODE ) ? DataType::String : DataType::Numeric;
        return InWorker(operator_node.right_expr, EvaluateVariant(data_type, operator_node.left_expr));
    }
}


double LogicInterpreter::ex_randomin(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const Nodes::In::Entry* in_node_entry = &GetNode<Nodes::In::Entry>(fnn_node.fn_expr[0]);

    struct RandomInRange
    {
        RandomInRange(double low_value_)
            :   low_value(low_value_),
                values_in_range(1)
        {
        }

        RandomInRange(int low_value_, int high_value_)
            :   low_value(low_value_),
                values_in_range(high_value_ - low_value_ + 1)
        {
        }

        double low_value;
        int values_in_range;
    };

    std::vector<RandomInRange> ranges;
    int total_number_values = 0;

    while( in_node_entry != nullptr )
    {
        // using a list or a value set
        if( in_node_entry->expression_low < 0 )
        {
            const Symbol& symbol = NPT_Ref(-1 * in_node_entry->expression_low);

            if( symbol.IsA(SymbolType::List) )
            {
                const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
                const size_t list_count = logic_list.GetCount();

                for( size_t i = 1; i <= list_count; ++i )
                {
                    ranges.emplace_back(logic_list.GetValue<double>(i));
                    ++total_number_values;
                }
            }

            else
            {
                const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);

                value_set.ForeachValue(
                    [&](const ValueSet::ForeachValueInfo& /*info*/, double low_value, const std::optional<double>& high_value)
                    {
                        if( !IsSpecial(low_value) )
                        {
                            if( !high_value.has_value() )
                            {
                                ranges.emplace_back(low_value);
                                ++total_number_values;
                            }

                            else if( !IsSpecial(*high_value) )
                            {
                                ASSERT(low_value <= *high_value);
                                ranges.emplace_back(static_cast<int>(low_value), static_cast<int>(*high_value));
                                total_number_values += ranges.back().values_in_range;
                            }
                        }
                    });
            }
        }

        // or a range
        else
        {
            const double low_value = Evaluate<double>(in_node_entry->expression_low);

            if( in_node_entry->expression_high < 0 )
            {
                ranges.emplace_back(low_value);
                ++total_number_values;
            }

            else
            {
                const double high_value = Evaluate<double>(in_node_entry->expression_high);

                if( low_value <= high_value && !IsSpecial(high_value) )
                {
                    ranges.emplace_back(static_cast<int>(low_value), static_cast<int>(high_value));
                    total_number_values += ranges.back().values_in_range;
                }
            }
        }

        in_node_entry = ( in_node_entry->next_entry_index != -1 ) ? &GetNode<Nodes::In::Entry>(in_node_entry->next_entry_index) :
                                                                    nullptr;
    }

    // get a random value within the ranges
    if( !ranges.empty() )
    {
        const int random_index = 1 + static_cast<int>(Randomizer::Next() * total_number_values);
        int number_values_passed_through = 0;

        // now find where the index is
        for( const RandomInRange& range : ranges )
        {
            if( ( number_values_passed_through + range.values_in_range ) >= random_index )
                return range.low_value - 1 + ( random_index - number_values_passed_through );

            number_values_passed_through += range.values_in_range;
        }
    }

    return DEFAULT;
}
