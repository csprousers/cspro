#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/EngineRecord.h>
#include <zCaseO/Case.h>


double CIntDriver::exmaxocc(int iExpr)
{
    if( m_iExSymbol <= 0 )
        return DEFAULT;

    // ENGINECR_TODO(maxocc) if maxocc() is called within a variable, its group is used for this calculation;
    // can we just change this to return the variable's max occurrences?
    const Symbol* symbol = NPT(m_iExSymbol);

    if( symbol->IsA(SymbolType::Variable) )
    {
        const GROUPT* pGroupT = assert_cast<const VART*>(symbol)->GetParentGPT();

        if( symbol != nullptr )
        {
            symbol = pGroupT;
        }

        else // Single var use owner
        {
            symbol = assert_cast<const VART*>(symbol)->GetOwnerGPT();
        }
    }

    return SymbolCalculator::GetMaximumOccurrences(*symbol);
}


double CIntDriver::exsoccurs(int iExpr)
{
    const auto& fn4_node = GetNode<FN4_NODE>(iExpr);
    const EngineRecord& engine_record = GetSymbolEngineRecord(fn4_node.sect_ind);

    // ENGINECR_TODO(soccurs) SECT's soccurs has a lot of logic ... do we need to carry this over?
    // some of it may be because noccurs(REC_NAME) becomes soccurs, so in entry
    // this might matter
    return engine_record.GetNumberOccurrences();
}
