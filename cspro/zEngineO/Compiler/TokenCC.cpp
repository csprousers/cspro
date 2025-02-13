#include "stdafx.h"
#include "IncludesCC.h"


DataType LogicCompiler::GetCurrentTokenDataType()
{
    // string literals
    if( Tkn == TOKSCTE )
        return DataType::String;

    // functions
    if( Tkn == TOKFUNCTION )
    {
        ASSERT(CurrentToken.function_details->return_data_type != DataType::Binary);
        return CurrentToken.function_details->return_data_type;
    }

    // symbols
    switch( Tkn )
    {
        case TOKARRAY:
        case TOKHASHMAP:
        case TOKLIST:
        case TOKUSERFUNCTION:
        case TOKVAR:
        case TOKWORKSTRING:
            return SymbolCalculator::GetDataType(NPT_Ref(Tokstindex));
    }

    // default to numeric
    return DataType::Numeric;
}
