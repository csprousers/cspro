#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "SelectDlgHelper.h"
#include <zEngineO/List.h>
#include <zUtilO/MemoryHelpers.h>
#include <zEngineO/SubscriptText.h>


#define EnsureListIsNotReadOnly(logic_list, return_value)                   \
    if( (logic_list).IsReadOnly() )                                         \
    {                                                                       \
        issaerror(MessageType::Error, 965, (logic_list).GetName().c_str()); \
        return return_value;                                                \
    }


std::optional<size_t> CIntDriver::EvaluateListIndex(int listvar_node_expression, LogicList** out_logic_list, bool for_assignment)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(listvar_node_expression);
    LogicList*& logic_list = *out_logic_list;

    logic_list = &GetSymbolLogicList(element_reference_node.symbol_index);

    if( for_assignment )
    {
        EnsureListIsNotReadOnly(*logic_list, std::nullopt);
    }

    size_t index = evalexpr<size_t>(element_reference_node.element_expressions[0]);

    if( logic_list->IsValidIndex(index) || ( for_assignment && index == ( logic_list->GetCount() + 1 ) ) )
        return index;

    issaerror(MessageType::Error, 1008, logic_list->GetName().c_str(), GetSubscriptText(index).c_str());
    return std::nullopt;
}


double CIntDriver::exlistvar(int iExpr)
{
    const LogicList* logic_list;
    std::optional<size_t> index = EvaluateListIndex(iExpr, const_cast<LogicList**>(&logic_list), false);

    if( !index.has_value() )
    {
        return AssignInvalidValue(logic_list->GetDataType());
    }

    else if( logic_list->IsNumeric() )
    {
        return logic_list->GetValue(*index);
    }

    else
    {
        return AssignAlphaValue(logic_list->GetString(*index));
    }
}


double CIntDriver::exlistcompute(int iExpr)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);

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

        const auto& list_values = GetListNode(symbol_compute_node.rhs_symbol_index);

        for( int i = 0; i < list_values.number_elements; ++i )
        {
            if( logic_list.IsNumeric() )
            {
                logic_list.AddValue(evalexpr(list_values.elements[i]));
            }

            else
            {
                logic_list.AddString(EvalAlphaExpr(list_values.elements[i]));
            }
        }
    }

    else
    {
        ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::None);
        LogicList* logic_list;
        std::optional<size_t> index = EvaluateListIndex(symbol_compute_node.lhs_symbol_index, &logic_list, true);

        if( !index.has_value() )
        {
            return DEFAULT;
        }

        else if( logic_list->IsNumeric() )
        {
            double value = evalexpr(symbol_compute_node.rhs_symbol_index);
            logic_list->SetValue(*index, value);
            return value;
        }

        else
        {
            logic_list->SetString(*index, EvalAlphaExpr(symbol_compute_node.rhs_symbol_index));
        }
    }

    return 0;
}


double CIntDriver::exlistadd(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    // adding a list
    if( symbol_va_node.arguments[0] == 1 )
    {
        const LogicList& logic_list_to_add = GetSymbolLogicList(symbol_va_node.arguments[1]);

        // you cannot add a list into itself
        if( &logic_list_to_add == &logic_list )
        {
            issaerror(MessageType::Error, 963, logic_list.GetName().c_str());
            return 0;
        }

        logic_list.InsertList(logic_list.GetCount() + 1, logic_list_to_add);

        return logic_list_to_add.GetCount();
    }

    // adding an item
    else
    {
        if( logic_list.IsNumeric() )
        {
            logic_list.AddValue(evalexpr(symbol_va_node.arguments[1]));
        }

        else
        {
            logic_list.AddString(EvalAlphaExpr(symbol_va_node.arguments[1]));
        }

        return 1;
    }
}


double CIntDriver::exlistclear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    logic_list.Reset();

    return 1;
}


double CIntDriver::exlistinsert(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    size_t index = evalexpr<size_t>(symbol_va_node.arguments[0]);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    if( !logic_list.IsValidIndex(index) && index != ( logic_list.GetCount() + 1 ) )
    {
        issaerror(MessageType::Error, 964, index, logic_list.GetName().c_str(), (int)logic_list.GetCount());
        return 0;
    }

    // inserting a list
    if( symbol_va_node.arguments[1] == 1 )
    {
        LogicList& logic_list_to_insert = GetSymbolLogicList(symbol_va_node.arguments[2]);

        // you cannot insert a list into itself
        if( &logic_list_to_insert == &logic_list )
        {
            issaerror(MessageType::Error, 963, logic_list.GetName().c_str());
            return 0;
        }

        logic_list.InsertList(index, logic_list_to_insert);

        return logic_list_to_insert.GetCount();
    }

    // inserting an item
    else
    {
        if( logic_list.IsNumeric() )
        {
            logic_list.InsertValue(index, evalexpr(symbol_va_node.arguments[2]));
        }

        else
        {
            logic_list.InsertString(index, EvalAlphaExpr(symbol_va_node.arguments[2]));
        }

        return 1;
    }
}


double CIntDriver::exlistlength(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    return logic_list.GetCount();
}


double CIntDriver::exlistremove(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    size_t index = evalexpr<size_t>(symbol_va_node.arguments[0]);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    if( !logic_list.IsValidIndex(index) )
    {
        issaerror(MessageType::Error, 964, index, logic_list.GetName().c_str(), (int)logic_list.GetCount());
        return 0;
    }

    logic_list.Remove(index);

    return 1;
}


double CIntDriver::exlistremoveduplicates(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    return logic_list.RemoveDuplicates();
}


double CIntDriver::exlistremovein(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    int in_node_expression = symbol_va_node.arguments[0];

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    // to ensure that expressions don't get evaluated over and over, we will cache the values here
    std::map<int, std::variant<double, std::wstring>> cached_values;

    std::function<const std::variant<double, std::wstring>&(int)> expression_evaluator =
        [&](int expression) -> const std::variant<double, std::wstring>&
        {
            const auto& cached_values_search = cached_values.find(expression);

            if( cached_values_search != cached_values.cend() )
                return cached_values_search->second;

            if( logic_list.IsNumeric() )
            {
                return cached_values.try_emplace(expression, evalexpr(expression)).first->second;
            }

            else
            {
                return cached_values.try_emplace(expression, EvalAlphaExpr(expression)).first->second;
            }
        };

    size_t number_removed = 0;

    for( size_t i = logic_list.GetCount(); i >= 1; --i )
    {
        bool found = logic_list.IsNumeric() ? InWorker(in_node_expression, logic_list.GetValue(i), &expression_evaluator) :
                                              InWorker(in_node_expression, logic_list.GetString(i), &expression_evaluator);

        if( found )
        {
            logic_list.Remove(i);
            ++number_removed;
        }
    }

    return number_removed;
}


namespace
{
    template<typename T, typename GVF>
    size_t ListSeekWorker(const LogicList& logic_list, GVF get_value_function, const T& value, size_t starting_index, size_t nth)
    {
        if( starting_index == 1 )
        {
            size_t index = logic_list.IndexOf(value);

            if( nth == 1 || index == 0 )
                return index;

            starting_index = index + 1;
            --nth;
        }

        size_t list_count = logic_list.GetCount();

        for( size_t i = starting_index; i <= list_count; ++i )
        {
            if( value == (logic_list.*get_value_function)(i) && --nth == 0 )
                return i;
        }

        return 0;
    }
}


double CIntDriver::exlistseek(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    size_t starting_index = ( symbol_va_node.arguments[1] >= 0 ) ? evalexpr<size_t>(symbol_va_node.arguments[1]) : 1;
    size_t nth = ( symbol_va_node.arguments[2] >= 0 ) ? evalexpr<size_t>(symbol_va_node.arguments[2]) : 1;

    if( logic_list.IsNumeric() )
    {
        return ListSeekWorker(logic_list, &LogicList::GetValue, evalexpr(symbol_va_node.arguments[0]), starting_index, nth);
    }

    else
    {
        return ListSeekWorker(logic_list, &LogicList::GetString, EvalAlphaExpr(symbol_va_node.arguments[0]), starting_index, nth);
    }
}


double CIntDriver::exlistshow(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exlistshow_pre77(iExpr);

    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    SelectDlg select_dlg(true, 1);

    if( symbol_va_node.arguments[0] != -1 )
        select_dlg.SetTitle(EvalAlphaExpr(symbol_va_node.arguments[0]));

    const size_t list_count = logic_list.GetCount();

    for( size_t i = 1; i <= list_count; ++i )
    {
        select_dlg.AddRow(logic_list.IsNumeric() ? DoubleToString(logic_list.GetValue(i)) :
                                                   logic_list.GetString(i));
    }

    return SelectDlgHelper(*this, select_dlg, Paradata::OperatorSelectionEvent::Source::ListShow).GetSingleSelection();
}


double CIntDriver::exlistshow_pre77(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);

    CString heading = ( symbol_va_node.arguments[0] != -1 ) ? EvalAlphaExpr<CString>(symbol_va_node.arguments[0]) :
                                                              CString();

    std::vector<std::vector<CString>*> data;

    for( size_t i = 1; i <= logic_list.GetCount(); ++i )
    {
        std::wstring value = logic_list.IsNumeric() ? DoubleToString(logic_list.GetValue(i)) :
                                                      logic_list.GetString(i);
            
        data.emplace_back(new std::vector<CString> { WS2CS(value) });
    }

    int selection = SelectDlgHelper_pre77(symbol_va_node.function_code, &heading, &data, nullptr, nullptr, nullptr);

    safe_delete_vector_contents(data);

    return selection;
}


double CIntDriver::exlistsort(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicList& logic_list = GetSymbolLogicList(symbol_va_node.symbol_index);
    bool ascending = ( symbol_va_node.arguments[0] == 0 );

    EnsureListIsNotReadOnly(logic_list, DEFAULT);

    logic_list.Sort(ascending);

    return 1;
}
