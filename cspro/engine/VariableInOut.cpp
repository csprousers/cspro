#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "Engine.h"
#include <zDictO/ValueProcessor.h>


// convert a engine value to its display value
double VARX::varoutval(double value) const
{
    if( !IsSpecial(value) )
        return value;

    const VART* pVarT = GetVarT();
    const auto& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    value = numeric_value_processor.ConvertNumberFromEngineFormat(value);

    if( value == NOTAPPL )
        value = MASKBLK;

    return value;
}


double VARX::varoutval(const CNDIndexes* pTheIndex) const
{
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    double value = ( pTheIndex == nullptr ) ?
        pIntDriver->GetVarFloatValue(const_cast<VARX*>(this)) :
        pIntDriver->GetVarFloatValue(const_cast<VARX*>(this), *pTheIndex);

    return varoutval(value);
}


// convert a display value to its engine value
double VARX::varinval(double value, const CNDIndexes* pTheIndex)
{
    const VART* pVarT = GetVarT();
    const auto& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();

    if( value == MASKBLK )
        value = NOTAPPL;

    value = numeric_value_processor.ConvertNumberToEngineFormat(value);

    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    if( pTheIndex == nullptr )
    {
        pIntDriver->SetVarFloatValueSingle(value, this);
    }

    else
    {
        pIntDriver->SetVarFloatValue(value, this, *pTheIndex);
    }

    return value;
}
