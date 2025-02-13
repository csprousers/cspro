#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/Messages/EngineMessages.h>
#include <zEngineO/Nodes/DataAccess.h>


double CIntDriver::exDataAccessValidityCheck(const int program_index)
{
    const auto& data_access_validity_check_node = GetNode<Nodes::DataAccessValidityCheck>(program_index);

    if constexpr(DebugMode())
    {
        const Symbol& check_symbol = NPT_Ref(data_access_validity_check_node.symbol_index);
        const EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(check_symbol);
        const Symbol* dictionary_based_symbol = ( engine_dictionary != nullptr )              ? engine_dictionary :
                                                check_symbol.IsA(SymbolType::Pre80Dictionary) ? &check_symbol:
                                                                                                nullptr;
        ASSERT(dictionary_based_symbol != nullptr && ( dictionary_based_symbol->GetSubType() == SymbolSubType::Input ||
                                                       dictionary_based_symbol->GetSubType() == SymbolSubType::Output ));
    }

    if( m_iExLevel < data_access_validity_check_node.symbol_level_number_base1 )
    {
        const Symbol& symbol = NPT_Ref(data_access_validity_check_node.symbol_index);
        issaerror(MessageType::Error, MGF::DataAccess_data_not_available_until_lower_level_94601, symbol.GetName().c_str());

        // return the invalid value
        return AssignInvalidValue(data_access_validity_check_node.function_return_data_type);
    }

    else
    {
        return evalexpr(data_access_validity_check_node.program_index);
    }
}


bool CIntDriver::IsDataAccessible(const Symbol& symbol, const bool issue_error_if_inaccessible)
{
    if( !SymbolCalculator::IsSymbolDataAccessible(symbol, m_iExLevel) )
    {
        //const EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(symbol); // ENGINECR_TODO restore when DICT is gone
        const Symbol* dictionary_based_symbol = SymbolCalculator(GetSymbolTable()).GetDicT(symbol);

        if( dictionary_based_symbol == nullptr )
        {
            dictionary_based_symbol = SymbolCalculator::GetEngineDictionary(symbol);                                                                                   

            if( dictionary_based_symbol == nullptr )
                return ReturnProgrammingError(true);
        }

        ASSERT(dictionary_based_symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary));

        // data in external dictionaries and working storage dictionaries can be accessed at any point [ENGINECR_TODO is this true of external forms?]
        if( dictionary_based_symbol->GetSubType() == SymbolSubType::Input || dictionary_based_symbol->GetSubType() == SymbolSubType::Output )
        {
            if( issue_error_if_inaccessible )
                issaerror(MessageType::Error, MGF::DataAccess_data_not_available_until_lower_level_94601, symbol.GetName().c_str());

            return false;
        }
    }

    return true;
}


void CIntDriver::EnsureDataIsAccessible(const Symbol& symbol)
{
    if( !IsDataAccessible(symbol, false) )
        m_pEngineDriver->GetSystemMessageIssuer().IssueOrThrow(true, MessageType::Error, MGF::DataAccess_data_not_available_until_lower_level_94601, symbol.GetName().c_str());
}
