#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/List.h>
#include <zEngineO/Pff.h>
#include <zEngineO/PffExecutor.h>
#include <zEngineO/Messages/EngineMessages.h>
#include <zUtilO/TemporaryFile.h>
#include <zBridgeO/NPff.h>


double CIntDriver::expffcompute(int iExpr)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);
    ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::Pff);

    LogicPff& lhs_logic_pff = GetSymbolLogicPff(symbol_compute_node.lhs_symbol_index);
    const LogicPff& rhs_logic_pff = GetSymbolLogicPff(symbol_compute_node.rhs_symbol_index);

    lhs_logic_pff = rhs_logic_pff;

    return 0;
}


double CIntDriver::expffload(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);
    std::wstring pff_filename;

    // when using the flow file name, load the current PFF
    if( symbol_va_node.arguments[0] < 0 )
    {
        pff_filename = CS2WS(m_pEngineDriver->m_pPifFile->GetPifFileName());
    }

    else
    {
        pff_filename = EvalAlphaExpr(symbol_va_node.arguments[0]);
    }

    MakeFullPathFileName(pff_filename);

    if( !PortableFunctions::FileExists(pff_filename) || !logic_pff.Load(pff_filename) )
    {
        issaerror(MessageType::Error, 47191, UTF8_TODO::GetUtf8(pff_filename).c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::expffsave(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    // make sure that the filename ends with .pff
    const std::string pff_file_path = PortableFunctions::PathEnsureFileExtension(EvaluatePath(symbol_va_node.arguments[0]),
                                                                                 FileExtensions::Pff);

    if( !logic_pff.Save(UTF8_TODO::GetWide(pff_file_path)) )
    {
        issaerror(MessageType::Error, 47192, pff_file_path.c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::expffgetproperty(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    std::wstring property_name = EvalAlphaExpr(symbol_va_node.arguments[0]);
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

    const std::vector<std::wstring> values_temp = logic_pff.GetProperties(property_name);
    std::vector<SharableString> values;
    for( const std::wstring& vt : values_temp ) values.emplace_back(UTF8_TODO::GetUtf8(vt));

    double return_value = !values.empty() ? AssignString(values.front()) :
                                            AssignStringNull();

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


double CIntDriver::expffsetproperty(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
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
            std::shared_ptr<const CDataDict> dictionary =
                symbol.IsA(SymbolType::Dictionary) ? assert_cast<const EngineDictionary&>(symbol).GetSharedDictionary() :
                                                     assert_cast<const DICT&>(symbol).GetSharedDictionary();

            // use the dictionary filename for the PFF value
            values.emplace_back(dictionary->GetFilePath());

            // set the embedded dictionary, issuing an error if the property name is invalid
            if( logic_pff.GetPffExecutor() == nullptr )
                logic_pff.SetPffExecutor(std::make_unique<PffExecutor>());

            if( !logic_pff.GetPffExecutor()->SetEmbeddedDictionary(UTF8_TODO::GetWide(*property_name), std::move(dictionary)) )
                issaerror(MessageType::Error, 47194, property_name->c_str());
        }
    }

    logic_pff.SetProperties(UTF8_TODO::GetWide(*property_name), UTF8_TODO::GetWide(values), CS2WS(m_pEngineDriver->m_pPifFile->GetAppFName()));

    return 1;
}


double CIntDriver::expffexec(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    return ExExecPFF(&logic_pff);
}
