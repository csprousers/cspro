//------------------------------------------------------------------------
//
//  ENGDRV.cpp
//
//------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "Engine.h"
#include "Comp.h"
#include <zEngineO/ResponseProcessor.h>
#include <zEngineO/ValueSet.h>
#include <ZBRIDGEO/npff.h>


void CEngineDriver::ResetDynamicAttributes(DICX* pDicX/* = nullptr*/)
{
    // if pDicX isn't null, then only the attributes related to that dictionary are affected
    bool bApplyToAllDictionaries = ( pDicX == nullptr );
    int iSymDic = 0;
    std::set<GROUPT*>* m_pGroupsSet = nullptr;

    if( !bApplyToAllDictionaries )
    {
        DICT* pDicT = pDicX->GetDicT();
        iSymDic = pDicT->GetSymbolIndex();

        if( Issamod == ModuleType::Entry )
            m_pGroupsSet = new std::set<GROUPT*>;
    }

    // reset the value sets for all of the fields to equal the first value set in the item
    for( VART* pVarT : m_engineData->variables )
    {
        if( !bApplyToAllDictionaries && pVarT->GetOwnerDic() != iSymDic )
            continue;

        pVarT->ResetCurrentValueSet();

        if( m_pGroupsSet != nullptr )
            m_pGroupsSet->insert(pVarT->GetOwnerGPT());
    }

    // reset each value set in case they were randomized
    for( ValueSet* value_set : m_engineData->value_sets_not_dynamic )
        value_set->GetResponseProcessor()->ResetResponses();

    // reset the occurrence visibilities and labels for each group
    for( GROUPT* pGroupT : m_engineData->groups )
    {
        // skip groups that belong to different dictionaries
        if( m_pGroupsSet != nullptr && m_pGroupsSet->find(pGroupT) == m_pGroupsSet->end() )
            continue;

        pGroupT->ResetOccVisibilities();
        pGroupT->ResetOccLabels();
    }

    delete m_pGroupsSet;
}
