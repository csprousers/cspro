#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineItem.h"
#include <engine/Dict.h>


namespace
{
    constexpr size_t MaxSpecifiableSubscripts  = 2;
    constexpr size_t RecordSubscriptIndex      = 0;
    constexpr size_t ItemSubitemSubscriptIndex = 1;
}


int LogicCompiler::CompileItemSubscriptExplicit(const EngineItem& engine_item)
{
    ASSERT(Tkn == TOKLPAREN);

    NextToken();

    if( Tkn == TOKRPAREN )
        IssueError(MGF::Item_subscript_cannot_be_empty_100400, engine_item.GetName().c_str());

    std::tuple<SubscriptValueType, int> subscripts[MaxSpecifiableSubscripts];
    size_t subscript_index = 0;

    while( true )
    {
        if( subscript_index == MaxSpecifiableSubscripts )
            IssueError(MGF::Item_subscript_too_many_subcripts_100401, static_cast<int>(MaxSpecifiableSubscripts), engine_item.GetName().c_str());

        SubscriptValueType& this_subscript_value_type = std::get<0>(subscripts[subscript_index]);

        if( Tkn == TOKCOMMA )
        {
            if( ( subscript_index + 1 ) == MaxSpecifiableSubscripts )
                IssueError(MGF::Item_subscript_separator_invalid_100403, engine_item.GetName().c_str());

            this_subscript_value_type = SubscriptValueType::Implicit;
            NextToken();
        }

        else if( Tkn == TOKRPAREN && ( subscript_index + 1 ) == MaxSpecifiableSubscripts )
        {
            this_subscript_value_type = SubscriptValueType::Implicit;
        }

        else if( Tkn == TOKCTE && IsNextToken({ TOKCOMMA, TOKRPAREN }) && IsNumericConstantInteger() )
        {
            this_subscript_value_type = SubscriptValueType::ConstantInteger;
            std::get<1>(subscripts[subscript_index]) = static_cast<int>(Tokvalue);
            NextToken();
        }

        else if( GetCurrentTokenDataType() == DataType::Numeric )
        {
            this_subscript_value_type = SubscriptValueType::Expression;
            std::get<1>(subscripts[subscript_index]) = exprlog();
        }

        else
        {
            IssueError(MGF::Item_subscript_type_invalid_100402, engine_item.GetName().c_str());
        }

        ++subscript_index;

        if( Tkn == TOKRPAREN )
            break;

        if( this_subscript_value_type != SubscriptValueType::Implicit )
        {
            if( Tkn != TOKCOMMA )
                IssueError(MGF::Item_subscript_separator_invalid_100403, engine_item.GetName().c_str());

            NextToken();
        }
    }

    ASSERT(subscript_index == 1 || subscript_index == 2);

    // if only one subscript was specified, set the other subscript as implicit
    if( subscript_index == 1 )
    {
        std::get<0>(subscripts[ItemSubitemSubscriptIndex]) = SubscriptValueType::Implicit;

        // if the item has item/subitem occurrences, swap the subscripts so that the implicit subscript is for the record occurrence
        if( engine_item.GetItemIndexHelper().HasItemSubitemOccurrences() )
            std::swap(subscripts[RecordSubscriptIndex], subscripts[ItemSubitemSubscriptIndex]);
    }

    ASSERT(Tkn == TOKRPAREN);

    NextToken();

    return ValidateItemSubscriptAndCreateNode<false>(engine_item, subscripts);
}


int LogicCompiler::CompileItemSubscriptImplicit(const EngineItem& engine_item, const Logic::FunctionDetails::StaticType static_type_of_use)
{
    static const std::tuple<SubscriptValueType, int> subscripts[] =
    {
        { SubscriptValueType::Implicit, 0 },
        { SubscriptValueType::Implicit, 0 },
    };

    static_assert(_countof(subscripts) == MaxSpecifiableSubscripts);

    if( static_type_of_use == Logic::FunctionDetails::StaticType::NeverStatic )
    {
        return ValidateItemSubscriptAndCreateNode<false>(engine_item, subscripts);
    }

    else
    {
        ASSERT(static_type_of_use == Logic::FunctionDetails::StaticType::StaticWhenNecessary);
        return ValidateItemSubscriptAndCreateNode<true>(engine_item, subscripts);
    }
}


template<bool fallback_to_static_compilation>
int LogicCompiler::ValidateItemSubscriptAndCreateNode(const EngineItem& engine_item, const std::tuple<SubscriptValueType, int> subscripts[])
{
    const ItemIndexHelper& item_index_helper = engine_item.GetItemIndexHelper();

    // validate constant integer subscripts
    auto validate_constant_integer_subscript = [&](const TCHAR* subscript_type, int specified_value, size_t max_value)
    {
        if( specified_value < 1 || specified_value > static_cast<int>(max_value) )
        {
            if( max_value == 1 )
            {
                IssueError(MGF::Item_subscript_must_be_one_100404, subscript_type, engine_item.GetName().c_str());
            }

            else
            {
                IssueError(MGF::Item_subscript_out_of_range_100405, subscript_type, engine_item.GetName().c_str(), static_cast<int>(max_value));
            }
        }
    };

    if( std::get<0>(subscripts[RecordSubscriptIndex]) == SubscriptValueType::ConstantInteger )
    {
        validate_constant_integer_subscript(_T("record"),
                                            std::get<1>(subscripts[RecordSubscriptIndex]),
                                            item_index_helper.GetMaxRecordOccurrences());
    }

    if( std::get<0>(subscripts[ItemSubitemSubscriptIndex]) == SubscriptValueType::ConstantInteger )
    {
        validate_constant_integer_subscript(item_index_helper.IsSubitem() ? _T("subitem") : _T("item"),
                                            std::get<1>(subscripts[ItemSubitemSubscriptIndex]),
                                            item_index_helper.GetMaxItemSubitemOccurrences());
    }

    const bool uses_implicit_subscripts = ( std::get<0>(subscripts[RecordSubscriptIndex]) == SubscriptValueType::Implicit &&
                                            std::get<0>(subscripts[ItemSubitemSubscriptIndex]) == SubscriptValueType::Implicit );

    ASSERT(!fallback_to_static_compilation || uses_implicit_subscripts);

    // when in a level-based PROC...
    if( !IsNoLevelCompilation() )
    {
        // ...we can check that the level is valid
        if( GetCompilationLevelNumber_base1() < SymbolCalculator::GetLevelNumber_base1(engine_item) )
        {
            const DICT* dict = SymbolCalculator(GetSymbolTable()).GetDicT(engine_item);
            ASSERT(dict != nullptr);

            // data in external dictionaries and working storage dictionaries can be accessed at any point [ENGINECR_TODO is this true of external forms?]
            if( dict->GetSubType() != SymbolSubType::External && dict->GetSubType() != SymbolSubType::Work )
            {
                if constexpr(fallback_to_static_compilation)
                {
                    return -1;
                }

                else
                {
                    IssueError(MGF::DataAccess_data_not_available_until_lower_level_94601, engine_item.GetName().c_str());
                }
            }
        }

        // ...and we can try to validate the implicit indices for multiply-occurring items
        if( uses_implicit_subscripts && item_index_helper.HasOccurrences() )
        {
            const std::vector<const DictNamedBase*> implicit_subscript_calculation_stack = GetImplicitSubscriptCalculationStack(engine_item);

            if( std::get<0>(subscripts[RecordSubscriptIndex]) == SubscriptValueType::Implicit && item_index_helper.HasRecordOccurrences() )
            {
                const auto& record_lookup = std::find(implicit_subscript_calculation_stack.cbegin(), implicit_subscript_calculation_stack.cend(),
                                                      engine_item.GetDictItem().GetRecord());

                if( record_lookup == implicit_subscript_calculation_stack.cend() )
                {
                    if constexpr(fallback_to_static_compilation)
                    {
                        return -1;
                    }

                    else
                    {
                        IssueError(MGF::Item_subscript_implicit_cannot_be_calculated_100406, _T("record"), engine_item.GetName().c_str());
                    }
                }
            }

            if( std::get<0>(subscripts[ItemSubitemSubscriptIndex]) == SubscriptValueType::Implicit && item_index_helper.HasItemSubitemOccurrences() )
            {
                ASSERT(item_index_helper.HasItemOccurrences() != item_index_helper.HasSubitemOccurrences());
                const CDictItem* dict_item_with_occurrences = &engine_item.GetDictItem();

                if( item_index_helper.IsSubitem() && item_index_helper.HasItemOccurrences() )
                    dict_item_with_occurrences = dict_item_with_occurrences->GetParentItem();

                ASSERT(dict_item_with_occurrences->GetOccurs() > 1);

                const auto& item_subitem_lookup = std::find(implicit_subscript_calculation_stack.cbegin(), implicit_subscript_calculation_stack.cend(),
                                                            dict_item_with_occurrences);

                if( item_subitem_lookup == implicit_subscript_calculation_stack.cend() )
                {
                    if constexpr(fallback_to_static_compilation)
                    {
                        return -1;
                    }

                    else
                    {
                        IssueError(MGF::Item_subscript_implicit_cannot_be_calculated_100406, item_index_helper.IsSubitem() ? _T("subitem") : _T("item"), engine_item.GetName().c_str());
                    }
                }
            }
        }
    }

    // create compilation nodes based on whether things were implicity or explicitly specified
    if( uses_implicit_subscripts )
    {
        auto& item_subscript_node = CreateVariableSizeNode<Nodes::ItemSubscript>(std::nullopt, 0);
        
        item_subscript_node.subscript_type = item_index_helper.HasOccurrences() ? Nodes::ItemSubscript::SubscriptType::ImplicitMustEvaluate :
                                                                                  Nodes::ItemSubscript::SubscriptType::ImplicitSinglyOccurring ;

        return GetProgramIndex(item_subscript_node);
    }

    else
    {
        auto& item_subscript_node = CreateVariableSizeNode<Nodes::ItemSubscript>(std::nullopt, MaxSpecifiableSubscripts * 2);

        item_subscript_node.subscript_type = Nodes::ItemSubscript::SubscriptType::Specified;

        item_subscript_node.subscript_arguments[0] = static_cast<int>(std::get<0>(subscripts[RecordSubscriptIndex]));
        item_subscript_node.subscript_arguments[1] = std::get<1>(subscripts[RecordSubscriptIndex]);
        item_subscript_node.subscript_arguments[2] = static_cast<int>(std::get<0>(subscripts[ItemSubitemSubscriptIndex]));
        item_subscript_node.subscript_arguments[3] = std::get<1>(subscripts[ItemSubitemSubscriptIndex]);

        return GetProgramIndex(item_subscript_node);
    }
}


int LogicCompiler::CompileItemFunctions()
{
    // compiling item_name.getValueLabel([visualValue := false, language := s]);
    // compiling item_name.hasValue([visualValue := false]);
    // compiling item_name.isValid([visualValue := false]);
    const Logic::FunctionDetails* function_details = CurrentToken.function_details;
    const EngineItem& engine_item = *assert_cast<const EngineItem*>(CurrentToken.symbol);
    ASSERT(CurrentToken.symbol_subscript_compilation != -1);

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_details->code,
                                                       engine_item, CurrentToken.symbol_subscript_compilation,
                                                       function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

    // process various named arguments
    int named_argument_index = 0;

    // "visualValue" (getValueLabel + hasValue + isValid)
    if( function_details->code == FunctionCode::ITEMFN_GETVALUELABEL_CODE ||
        function_details->code == FunctionCode::ITEMFN_HASVALUE_CODE ||
        function_details->code == FunctionCode::ITEMFN_ISVALID_CODE )
    {
        optional_named_arguments_compiler.AddArgument(_T("visualValue"), symbol_va_with_subscript_node.arguments[named_argument_index++], DataType::Numeric);
    }

    // "language" (getValueLabel)
    if( function_details->code == FunctionCode::ITEMFN_GETVALUELABEL_CODE )
    {
        optional_named_arguments_compiler.AddArgument(_T("language"), symbol_va_with_subscript_node.arguments[named_argument_index++], DataType::String);
    }

    ASSERT(named_argument_index == function_details->number_arguments);

    if( optional_named_arguments_compiler.Compile(true) == 0 )
        NextToken();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
