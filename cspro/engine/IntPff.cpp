#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/List.h>
#include <zEngineO/Pff.h>
#include <zEngineO/PffExecutor.h>
#include <zEngineO/Versioning.h>
#include <zUtilO/TemporaryFile.h>
#include <ZBRIDGEO/npff.h>


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
        issaerror(MessageType::Error, 47191, pff_filename.c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::expffsave(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    // make sure that the filename ends with .pff
    std::wstring pff_filename = PortableFunctions::PathEnsureFileExtension(EvalFullPathFileName(symbol_va_node.arguments[0]),
                                                                           FileExtensions::Pff);

    if( !logic_pff.Save(pff_filename) )
    {
        issaerror(MessageType::Error, 47192, pff_filename.c_str());
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

    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        logic_list = ( symbol_va_node.arguments[1] != -1 ) ? &GetSymbolLogicList(symbol_va_node.arguments[1]) :
                                                             nullptr;
    }

    else
    {
        logic_list = ( symbol_va_node.arguments[1] < 0 ) ? &GetSymbolLogicList(-1 * symbol_va_node.arguments[1]) :
                                                           nullptr;
    }

    std::vector<std::wstring> values = logic_pff.GetProperties(property_name);

    if( logic_list != nullptr )
    {
        if( logic_list->IsReadOnly() )
        {
            issaerror(MessageType::Error, 965, logic_list->GetName().c_str());
        }

        else
        {
            logic_list->Reset();
            logic_list->AddStrings(values);
        }
    }

    return !values.empty() ? AssignAlphaValue(values.front()) :
                             AssignBlankAlphaValue();
}


double CIntDriver::expffsetproperty(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    std::wstring property_name = EvalAlphaExpr(symbol_va_node.arguments[0]);
    std::vector<std::wstring> values;

    if( symbol_va_node.arguments[1] >= 0 )
    {
        if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_7_000_1) )
        {
            values.emplace_back(EvalAlphaExpr(symbol_va_node.arguments[1]));
        }

        else
        {
            values.emplace_back(EvaluateExpressionAsString(static_cast<DataType>(symbol_va_node.arguments[1]), symbol_va_node.arguments[2]));
        }
    }

    else
    {
        const Symbol& symbol = NPT_Ref(-1 * symbol_va_node.arguments[1]);

        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
            size_t list_count = logic_list.GetCount();

            for( size_t i = 1; i <= list_count; ++i )
                values.emplace_back(logic_list.GetString(i));
        }

        else
        {
            ASSERT(symbol.IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary));
            std::shared_ptr<const CDataDict> dictionary =
                symbol.IsA(SymbolType::Dictionary) ? assert_cast<const EngineDictionary&>(symbol).GetSharedDictionary() :
                                                     assert_cast<const DICT&>(symbol).GetSharedDictionary();

            // use the dictionary filename for the PFF value
            values.emplace_back(CS2WS(dictionary->GetFullFileName()));

            // set the embedded dictionary, issuing an error if the property name is invalid
            if( logic_pff.GetPffExecutor() == nullptr )
                logic_pff.SetPffExecutor(std::make_unique<PffExecutor>());

            if( !logic_pff.GetPffExecutor()->SetEmbeddedDictionary(property_name, dictionary) )
                issaerror(MessageType::Error, 47194, property_name.c_str());
        }
    }

    logic_pff.SetProperties(property_name, values, CS2WS(m_pEngineDriver->m_pPifFile->GetAppFName()));

    return 1;
}


double CIntDriver::expffexec(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicPff& logic_pff = GetSymbolLogicPff(symbol_va_node.symbol_index);

    return ExExecPFF(&logic_pff);
}
