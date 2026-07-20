#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineDictionary.h"
#include "List.h"
#include "Pff.h"
#include "PffExecutor.h"
#include <engine/DicT.h>


double LogicInterpreter::ex_Pff_compute(const int program_index)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);
    ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::Pff);

    LogicPff& lhs_logic_pff = GetSymbolLogicPff(symbol_compute_node.lhs_symbol_index);
    const LogicPff& rhs_logic_pff = GetSymbolLogicPff(symbol_compute_node.rhs_symbol_index);

    lhs_logic_pff = rhs_logic_pff;

    return 0;
}


double LogicInterpreter::ex_Pff_load(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);
    std::string pff_file_path;

    // when using the flow file name, load the current PFF
    if( symbol_va_node.arguments[0] < 0 )
    {
        if( m_engineData->pff != nullptr )
            pff_file_path = GetAbsolutePath(UTF8_TODO::GetUtf8(m_engineData->pff->GetPifFileName()));

        ASSERT(!pff_file_path.empty());
    }

    else
    {
        pff_file_path = EvaluatePath(symbol_va_node.arguments[0]);
    }

    if( pff_file_path.empty() ||
        !PortableFunctions::FileIsRegular(pff_file_path) ||
        !logic_pff.Load(pff_file_path) )
    {
        IssueMessage(MessageType::Error, MGF::Pff_load_error_47191, pff_file_path.c_str());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Pff_save(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    // make sure that the filename ends with .pff
    const std::string pff_file_path = PortableFunctions::PathEnsureFileExtension(
        EvaluatePath(symbol_va_node.arguments[0]),
        FileExtensions::Pff
    );

    if( !logic_pff.Save(pff_file_path) )
    {
        IssueMessage(MessageType::Error, MGF::Pff_save_error_47192, pff_file_path.c_str());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Pff_getProperty(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    const SharableString property_name = EvaluateSharableString(symbol_va_node.arguments[0]);
    LogicList* logic_list;

    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        logic_list = ( symbol_va_node.arguments[1] != -1 ) ? &GetSymbolLogicList(symbol_va_node.arguments[1]) :
                                                             nullptr;
    }

    else
    {
        logic_list = ( symbol_va_node.arguments[1] < 0 ) ? &GetSymbolLogicList(-1 * symbol_va_node.arguments[1]) :
                                                           nullptr;
    }

    std::vector<std::string> values = logic_pff.GetProperties(*property_name);

    const double return_value = !values.empty()
        ? AssignString(values.front())
        : AssignStringNull();

    if( logic_list != nullptr )
    {
        if( logic_list->IsReadOnly() )
        {
            IssueMessage(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, logic_list->GetName().c_str());
        }

        else
        {
            logic_list->Reset();
            logic_list->AddValues(std::move(values));
        }
    }

    return return_value;
}


double LogicInterpreter::ex_Pff_setProperty(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    const SharableString property_name = EvaluateSharableString(symbol_va_node.arguments[0]);
    std::vector<SharableString> values;

    if( symbol_va_node.arguments[1] >= 0 )
    {
        values.emplace_back(EvaluateSharableString(static_cast<DataType>(symbol_va_node.arguments[1]),
                                                   symbol_va_node.arguments[2]));
    }

    else
    {
        const Symbol& symbol = NPT_Ref(-1 * symbol_va_node.arguments[1]);

        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
            const size_t list_count = logic_list.GetCount();

            for( size_t i = 1; i <= list_count; ++i )
                values.emplace_back(logic_list.GetValue<SharableString>(i));
        }

        else
        {
            ASSERT(symbol.IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary));

            std::shared_ptr<const CDataDict> dictionary = symbol.IsA(SymbolType::Dictionary)
                ? assert_cast<const EngineDictionary&>(symbol).GetSharedDictionary()
                : assert_cast<const DICT&>(symbol).GetSharedDictionary();

            // use the dictionary file path for the PFF value
            values.emplace_back(dictionary->GetFilePath());

            // set the embedded dictionary, issuing an error if the property name is invalid
            if( logic_pff.GetPffExecutor() == nullptr )
                logic_pff.SetPffExecutor(std::make_unique<PffExecutor>());

            if( !logic_pff.GetPffExecutor()->SetEmbeddedDictionary(*property_name, std::move(dictionary)) )
                IssueMessage(MessageType::Error, MGF::Pff_property_invalid_with_dictionary_47194, property_name->c_str());
        }
    }

    logic_pff.SetProperties(*property_name, values, GetCurrentApplicationFilePath());

    return 1;
}


double LogicInterpreter::ex_Pff_exec(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    return ExExecPFF_INTERPRETER_DLL_TODO(logic_pff);
}
