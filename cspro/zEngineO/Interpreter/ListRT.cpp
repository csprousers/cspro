#include "stdafx.h"
#include "IncludesRT.h"
#include "List.h"
#include "SelectDlgHelper.h"
#include "SubscriptText.h"


#define EnsureListIsNotReadOnly(logic_list, return_value)                            \
{                                                                                    \
    if( (logic_list).IsReadOnly() )                                                  \
    {                                                                                \
        IssueMessage(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, \
                                         (logic_list).GetName().c_str());            \
        return return_value;                                                         \
    }                                                                                \
}


std::optional<size_t> LogicInterpreter::EvaluateListIndex(const int listvar_node_expression, LogicList** const out_logic_list, const bool for_assignment)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(listvar_node_expression);
    LogicList*& logic_list = *out_logic_list;

    logic_list = &GetSymbolLogicList(element_reference_node.symbol_index);

    if( for_assignment )
        EnsureListIsNotReadOnly(*logic_list, std::nullopt);

    const size_t index = Evaluate<size_t>(element_reference_node.element_expressions[0]);

    if( logic_list->IsValidIndex(index) || ( for_assignment && index == ( logic_list->GetCount() + 1 ) ) )
        return index;

    IssueMessage(MessageType::Error, MGF::invalid_subscript_1008, logic_list->GetName().c_str(), GetSubscriptText(index).c_str());
    return std::nullopt;
}


double LogicInterpreter::ex_List_var(const int program_index)
{
    const LogicList* logic_list;
    const std::optional<size_t> index = EvaluateListIndex(program_index, const_cast<LogicList**>(&logic_list), false);

    if( !index.has_value() )
    {
        return AssignInvalidValue(logic_list->GetDataType());
    }

    else if( logic_list->IsNumeric() )
    {
        return logic_list->GetValue<double>(*index);
    }

    else
    {
        return AssignString(logic_list->GetValue<SharableString>(*index));
    }
}


double LogicInterpreter::ex_List_compute(const int program_index)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);

    if( symbol_compute_node.rhs_symbol_type == SymbolType::List )
    {
        LogicList& lhs_logic_list = GetSymbolLogicList(symbol_compute_node.lhs_symbol_index);
        const LogicList& rhs_logic_list = GetSymbolLogicList(symbol_compute_node.rhs_symbol_index);

        EnsureListIsNotReadOnly(lhs_logic_list, DEFAULT);

        // only do the assignment if they're not assigning a list to itself
        if( &lhs_logic_list != &rhs_logic_list )
        {
            lhs_logic_list.Reset();
            lhs_logic_list.InsertList(1, rhs_logic_list);
        }
    }

    else if( symbol_compute_node.rhs_symbol_type == SymbolType::Variable )
    {
        LogicList& logic_list = GetSymbolLogicList(symbol_compute_node.lhs_symbol_index);

        EnsureListIsNotReadOnly(logic_list, DEFAULT);

        logic_list.Reset();

        const Nodes::List& list_values = GetListNode(symbol_compute_node.rhs_symbol_index);

        for( int i = 0; i < list_values.number_elements; ++i )
        {
            logic_list.IsNumeric() ? logic_list.AddValue(Evaluate(list_values.elements[i])) :
                                     logic_list.AddValue(EvaluateSharableString(list_values.elements[i]));
        }
    }

    else
    {
        ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::None);
        LogicList* logic_list;
        const std::optional<size_t> index = EvaluateListIndex(symbol_compute_node.lhs_symbol_index, &logic_list, true);

        if( !index.has_value() )
        {
            return DEFAULT;
        }

        else if( logic_list->IsNumeric() )
        {
            double value = Evaluate(symbol_compute_node.rhs_symbol_index);
            logic_list->SetValue(*index, value);
            return value;
        }

        else
        {
            logic_list->SetValue(*index, EvaluateSharableString(symbol_compute_node.rhs_symbol_index));
        }
    }

    return 0;
}


double LogicInterpreter::ex_List_add(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    // adding a list
    if( symbol_va_node.arguments[0] == 1 )
    {
        const LogicList& logic_list_to_add = GetSymbolLogicList(symbol_va_node.arguments[1]);

        // you cannot add a list into itself
        if( &logic_list_to_add == &logic_list )
        {
            IssueMessage(MessageType::Error, MGF::List_cannot_assign_to_itself_963, logic_list.GetName().c_str());
            return 0;
        }

        logic_list.InsertList(logic_list.GetCount() + 1, logic_list_to_add);

        return static_cast<double>(logic_list_to_add.GetCount());
    }

    // adding an item
    else
    {
        logic_list.IsNumeric() ? logic_list.AddValue(Evaluate(symbol_va_node.arguments[1])) :
                                 logic_list.AddValue(EvaluateSharableString(symbol_va_node.arguments[1]));

        return 1;
    }
}


double LogicInterpreter::ex_List_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    logic_list.Reset();

    return 1;
}


double LogicInterpreter::ex_List_insert(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    const size_t index = Evaluate<size_t>(symbol_va_node.arguments[0]);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    if( !logic_list.IsValidIndex(index) && index != ( logic_list.GetCount() + 1 ) )
    {
        IssueMessage(MessageType::Error, MGF::List_invalid_index_964, index, logic_list.GetName().c_str(), static_cast<int>(logic_list.GetCount()));
        return 0;
    }

    // inserting a list
    if( symbol_va_node.arguments[1] == 1 )
    {
        LogicList& logic_list_to_insert = GetSymbolLogicList(symbol_va_node.arguments[2]);

        // you cannot insert a list into itself
        if( &logic_list_to_insert == &logic_list )
        {
            IssueMessage(MessageType::Error, MGF::List_cannot_assign_to_itself_963, logic_list.GetName().c_str());
            return 0;
        }

        logic_list.InsertList(index, logic_list_to_insert);

        return static_cast<double>(logic_list_to_insert.GetCount());
    }

    // inserting an item
    else
    {
        logic_list.IsNumeric() ? logic_list.InsertValue(index, Evaluate(symbol_va_node.arguments[2])) :
                                 logic_list.InsertValue(index, EvaluateSharableString(symbol_va_node.arguments[2]));

        return 1;
    }
}


double LogicInterpreter::ex_List_length(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    return static_cast<double>(logic_list.GetCount());
}


double LogicInterpreter::ex_List_remove(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    const size_t index = Evaluate<size_t>(symbol_va_node.arguments[0]);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    if( !logic_list.IsValidIndex(index) )
    {
        IssueMessage(MessageType::Error, MGF::List_invalid_index_964, index, logic_list.GetName().c_str(), static_cast<int>(logic_list.GetCount()));
        return 0;
    }

    logic_list.Remove(index);

    return 1;
}


double LogicInterpreter::ex_List_removeDuplicates(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    return static_cast<double>(logic_list.RemoveDuplicates());
}


double LogicInterpreter::ex_List_removeIn(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    const int in_node_expression = symbol_va_node.arguments[0];

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    // to ensure that expressions don't get evaluated over and over, we will cache the values here
    std::map<int, std::variant<double, SharableString>> cached_values;

    const std::function<const std::variant<double, SharableString>&(int)> expression_evaluator =
        [&](const int expression) -> const std::variant<double, SharableString>&
        {
            auto cached_values_search = cached_values.find(expression);

            if( cached_values_search == cached_values.cend() )
                cached_values_search = cached_values.try_emplace(expression, EvaluateVariant(logic_list.GetDataType(), expression)).first;

            return cached_values_search->second;
        };

    size_t number_removed = 0;

    for( size_t i = logic_list.GetCount(); i >= 1; --i )
    {
        const bool found = logic_list.IsNumeric() ? InWorker(in_node_expression, logic_list.GetValue<double>(i), &expression_evaluator) :
                                                    InWorker(in_node_expression, logic_list.GetValue<SharableString>(i), &expression_evaluator);

        if( found )
        {
            logic_list.Remove(i);
            ++number_removed;
        }
    }

    return static_cast<double>(number_removed);
}


double LogicInterpreter::ex_List_seek(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    size_t starting_index = ( symbol_va_node.arguments[1] >= 0 ) ? Evaluate<size_t>(symbol_va_node.arguments[1]) : 1;
    size_t nth = ( symbol_va_node.arguments[2] >= 0 ) ? Evaluate<size_t>(symbol_va_node.arguments[2]) : 1;

    const std::variant<double, SharableString> value = EvaluateVariant(logic_list.GetDataType(), symbol_va_node.arguments[0]);

    if( starting_index == 1 )
    {
        const size_t index = std::visit([&](const auto& this_value) { return logic_list.IndexOf(this_value); }, value);

        if( nth == 1 || index == 0 )
            return static_cast<double>(index);

        starting_index = index + 1;
        --nth;
    }

    const size_t list_count = logic_list.GetCount();

    for( size_t i = starting_index; i <= list_count; ++i )
    {
        const bool value_equals = logic_list.IsNumeric() ? ( std::get<double>(value) == logic_list.GetValue<double>(i) ) :
                                                           ( std::get<SharableString>(value) == logic_list.GetValue<SharableString>(i) );

        if( value_equals && --nth == 0 )
            return static_cast<double>(i);
    }

    return 0;
}


double LogicInterpreter::ex_List_show(const int program_index)
{
    if( !UseHtmlDialogs() )
        return ex_List_show_pre77(program_index);

    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    SelectDlg select_dlg(true, 1);

    if( symbol_va_node.arguments[0] != -1 )
        select_dlg.SetTitle(EvaluateSharableString(symbol_va_node.arguments[0]));

    const size_t list_count = logic_list.GetCount();

    if( logic_list.IsNumeric() )
    {
        for( size_t i = 1; i <= list_count; ++i )
            select_dlg.AddRow(DoubleToString(logic_list.GetValue<double>(i)));
    }

    else
    {
        for( size_t i = 1; i <= list_count; ++i )
            select_dlg.AddRow(logic_list.GetValue<SharableString>(i));
    }

    SelectDlgHelper select_dlg_helper(GetEngineParadataDriver_INTERPRETER_DLL_TODO(), select_dlg, Paradata::OperatorSelectionEvent::Source::ListShow);
    return static_cast<double>(select_dlg_helper.GetSingleSelection());
}


double LogicInterpreter::ex_List_show_pre77(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    const SharableString heading = EvaluateNullableSharableString(symbol_va_node.arguments[0]);
    std::vector<std::vector<CString>*> data;

    for( size_t i = 1; i <= logic_list.GetCount(); ++i )
    {
        const SharableString value = logic_list.IsNumeric() ? DoubleToString(logic_list.GetValue<double>(i)) :
                                                              logic_list.GetValue<SharableString>(i);

        data.emplace_back(new std::vector<CString> { UTF8_TODO::GetCString(*value) });
    }

    const int selection = SelectDlgHelper_pre77(symbol_va_node.function_code, UTF8_TODO::GetCString(heading), &data, nullptr, nullptr, nullptr);

    for( const std::vector<CString>* const d : data )
        delete d;

    return selection;
}


double LogicInterpreter::ex_List_sort(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    const bool ascending = ( symbol_va_node.arguments[0] == 0 );

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    logic_list.Sort(ascending);

    return 1;
}
