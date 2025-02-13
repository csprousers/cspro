#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/DataAccess.h"
#include <zEngineO/EngineDictionary.h>


int LogicCompiler::WrapNodeAroundValidDataAccessCheck(const int program_index, const Symbol& symbol,
                                                      const std::variant<FunctionCode, DataType> function_code_or_data_type)
{
    // accessing data from an input/special output dictionary is only valid in certain places;
    // instead of having to check for valid access in many compiler/runtime places, we can wrap a
    // function call-style node around a check that confirms (at compile- and runtime) if the access
    // is valid; if the access is always valid, then no new node is created; if at compile-time the
    // access is invalid, a compiler error is issued; if the access is potentially fine, then at runtime
    // the check will be made and, if the access is invalid, the return value will be the result of
    // AssignInvalidValue called with the function's return type (or data type, whichever is passed)

    const EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(symbol);
    const Symbol* dictionary_based_symbol = ( engine_dictionary != nullptr )        ? engine_dictionary :
                                            symbol.IsA(SymbolType::Pre80Dictionary) ? &symbol :
                                                                                      nullptr;
    bool requires_check;

    // non-dictionary-based symbols don't require special checks
    if( dictionary_based_symbol == nullptr)
    {
        ASSERT(SymbolCalculator::GetDictBase(symbol) == nullptr);
        requires_check = false;
    }

    // data in external dictionaries and working storage dictionaries can be accessed at any point [ENGINECR_TODO is this true of external forms?]
    else if( dictionary_based_symbol->GetSubType() == SymbolSubType::External || dictionary_based_symbol->GetSubType() == SymbolSubType::Work )
    {
        requires_check = false;
    }

    // if in an unknown level, then we always need to check input/special output data at runtime
    else if( IsNoLevelCompilation() )
    {
        requires_check = true;
    }

    // if in a proc, we have to check if the data is available at this level only at compile-time
    else 
    {
        if( GetCompilationLevelNumber_base1() < SymbolCalculator::GetLevelNumber_base1(symbol) )
            IssueError(MGF::DataAccess_data_not_available_until_lower_level_94601, symbol.GetName().c_str());

        requires_check = false;
    }


    if( !requires_check )
        return program_index;

    auto& data_access_validity_check_node = CreateNode<Nodes::DataAccessValidityCheck>(FunctionCode::DATA_ACCESS_VALIDITY_CHECK_CODE);

    data_access_validity_check_node.program_index = program_index;
    data_access_validity_check_node.symbol_index = symbol.GetSymbolIndex();
    data_access_validity_check_node.symbol_level_number_base1 = SymbolCalculator::GetLevelNumber_base1(symbol);

    if( std::holds_alternative<DataType>(function_code_or_data_type) )
    {
        data_access_validity_check_node.function_return_data_type = std::get<DataType>(function_code_or_data_type);
    }

    else
    {
        const Logic::FunctionDetails* function_details = Logic::FunctionTable::GetFunctionDetails(std::get<FunctionCode>(function_code_or_data_type));

        // default to a numeric return value if this function code has no details
        // (which can occur, for instance, with symbol assignment nodes)
        data_access_validity_check_node.function_return_data_type = ( function_details != nullptr ) ? function_details->return_data_type :
                                                                                                      DataType::Numeric;
    }

    return GetProgramIndex(data_access_validity_check_node);
}
