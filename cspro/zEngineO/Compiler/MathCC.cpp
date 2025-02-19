#include "stdafx.h"
#include "IncludesCC.h"


bool LogicCompiler::IsNumericConstantInteger(double value)
{
    return ( floorl(value) == value );
}


bool LogicCompiler::IsNumericConstantInteger() const
{
    ASSERT(Tkn == TOKCTE);
    return IsNumericConstantInteger(Tokvalue);
}
