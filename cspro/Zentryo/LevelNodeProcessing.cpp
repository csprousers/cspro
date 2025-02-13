#include "StdAfx.h"
#include <engine/Engine.h>
#include <zCaseO/Case.h>


int CEntryDriver::GetTotalNumberNodes()
{
    Pre74_Case* pCase = GetInputCase().GetPre74_Case();
    return pCase->GetNumberNodes();
}


int CEntryDriver::GetNodeNumber(Pre74_CaseLevel* pCaseLevel)
{
    int iNodeNum = -1;

    if( pCaseLevel != nullptr )
    {
        Pre74_Case* pCase = GetInputCase().GetPre74_Case();
        iNodeNum = pCase->GetCaseLevelNodeNumber(pCaseLevel);
    }

    return iNodeNum;
}


Pre74_CaseLevel* CEntryDriver::GetCaseLevelNode(int iLevel)
{
    return ( iLevel >= 1 ) ? m_aLoadedLevels[iLevel - 1].m_caseLevel : nullptr;
}


bool CEntryDriver::IsLevelNodeNew(int iLevel) const
{
    return m_aLoadedLevels[iLevel - 1].m_isNew;
}


void CEntryDriver::AddLevelNode(int iLevel)
{
    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();

    Pre74_CaseLevel* pAddedCaseLevel = nullptr;

    if( iLevel == 1 )
    {
        Pre74_Case* pCase = pDicX->GetCase().GetPre74_Case();
        pAddedCaseLevel = pCase->GetRootLevel();
    }

    else
    {
        Pre74_CaseLevel* pParentLevel = m_aLoadedLevels[iLevel - 2].m_caseLevel;
        const DictLevel& dict_level = pDicT->GetDataDict()->GetLevel(iLevel - 1);
        pAddedCaseLevel = pParentLevel->AddChildLevel(dict_level);
    }

    UpdateLoadedLevels(pAddedCaseLevel,true);
}


void CEntryDriver::GetNextLevelNode(int iSearchedLevel)
{
    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();

    Case& data_case = pDicX->GetCase();
    Pre74_Case* pCase = data_case.GetPre74_Case();
    Pre74_CaseLevel* pLevelToLoad = nullptr;
    Pre74_CaseLevel* pLoadedLevel = m_aLoadedLevels[iSearchedLevel - 1].m_caseLevel;

    if( iSearchedLevel == 1 ) // the root level
    {
        pLevelToLoad = pCase->GetRootLevel();
        m_setUpdatedLevelNodes.clear();
    }

    else
    {
        // if this is the first node at the level (if 2+), then this will set the following code to look for
        // a child level instead of another sibling level
        if( pLoadedLevel == nullptr )
            pLoadedLevel = m_aLoadedLevels[iSearchedLevel - 2].m_caseLevel;

        // the next node might be a child one
        if( pLoadedLevel->GetLevelNum() < iSearchedLevel )
        {
            if( pLoadedLevel->GetNumChildLevels() > 0 )
                pLevelToLoad = pLoadedLevel->GetChildLevel(0);
        }

        // the next node might be a sibling
        else
        {
            // search for the previously loaded level and then get the next one (if applicable)
            Pre74_CaseLevel* pParentLevel = pCase->FindParentLevel(pLoadedLevel);

            if( pParentLevel != nullptr )
            {
                bool bFoundLevel = false;

                for( int i = 0; i < pParentLevel->GetNumChildLevels(); i++ )
                {
                    Pre74_CaseLevel* pChildLevel = pParentLevel->GetChildLevel(i);

                    if( bFoundLevel )
                    {
                        pLevelToLoad = pChildLevel;
                        break;
                    }

                    else if( pChildLevel == pLoadedLevel )
                    {
                        bFoundLevel = true;
                    }
                }
            }
        }
    }

    if( pLevelToLoad == nullptr )
    {
        AddLevelNode(iSearchedLevel);
    }

    else
    {
        // BINARY_TYPES_TO_ENGINE_TODO temporary processing for 8.0
        // find the CaseLevel with the binary data
        const CaseLevel* case_level_with_binary_data = nullptr;

        if( iSearchedLevel == 1 )
        {
            case_level_with_binary_data = &data_case.GetRootCaseLevel();
        }

        else
        {
            for( const CaseLevel* case_level : data_case.GetAllCaseLevels() )
            {
                if( &case_level->GetCaseLevelMetadata().GetDictLevel() == &pLevelToLoad->GetDictLevel() )
                {
                    const CString full_key_CaseLevel = data_case.GetKey() + case_level->GetLevelIdentifier();
                    const CString full_key_Pre74_CaseLevel = pLevelToLoad->GetKey();

                    if( full_key_CaseLevel == full_key_Pre74_CaseLevel )
                    {
                        case_level_with_binary_data = case_level;
                        break;
                    }
                }
            }

        }

        ParseCaseLevel(&pDicX->GetCase(), pLevelToLoad, case_level_with_binary_data, pDicT);
    }
}


void CEntryDriver::UpdateLevelNode(int iEndingLevel, bool bPartialSave/* = false*/, Case* data_case/* = nullptr*/)
{
    DICT* pDicT = DIP(0);
    Pre74_Case* pCase;
    Pre74_CaseLevel* pLevelToUpdate;

    // normal processing
    if( data_case == nullptr )
    {
        data_case = &pDicT->GetDicX()->GetCase();
        pCase = data_case->GetPre74_Case();
        pLevelToUpdate = m_aLoadedLevels[iEndingLevel - 1].m_caseLevel;
    }

    // questionnaire view processing
    else
    {
        if( iEndingLevel > 1 )
        {
            return; // VQ_TODO support multiple level cases at some point?
        }

        pCase = data_case->GetPre74_Case();
        pLevelToUpdate = pCase->GetRootLevel();
    }

    CopyLevelToRepository(pDicT, data_case, pLevelToUpdate, bPartialSave);

    // required records for the first level will get added in WriteData, PartialSaveCase, or exwritecase
    bool bAddRequiredRecords = ( iEndingLevel > 1 );

    pCase->FinalizeLevel(pLevelToUpdate,bAddRequiredRecords,false,nullptr);

    m_setUpdatedLevelNodes.insert(pLevelToUpdate);
}

void CEntryDriver::UpdateLevelNodeAtEnd(int iEndingLevel)
{
    // this function is similar to the above, except that it doesn't add the level to the
    // set of updated level nodes; if ClearUnprocessedLevels is called at some point, this node will be
    // removed, which makes sense in the case of, for example, an endlevel statement called from logic,
    // as opposed to an endlevel being executed by the operator
    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();
    Pre74_Case* pCase = pDicX->GetCase().GetPre74_Case();

    Pre74_CaseLevel* pLevelToUpdate = m_aLoadedLevels[iEndingLevel - 1].m_caseLevel;

    CopyLevelToRepository(pDicT, &pDicX->GetCase(), pLevelToUpdate);

    try
    {
        pCase->FinalizeLevel(pLevelToUpdate,true,true,nullptr);
    }

    catch( CaseHasNoValidRecordsException )
    {
    }
}


void CEntryDriver::ClearUnprocessedLevels(int iLevel)
{
    if( iLevel < 2 )
        return;

    Pre74_CaseLevel* pParentLevel = m_aLoadedLevels[iLevel - 2].m_caseLevel;

    for( int i = pParentLevel->GetNumChildLevels() - 1; i >= 0; i-- )
    {
        Pre74_CaseLevel* pChildLevel = pParentLevel->GetChildLevel(i);

        if( m_setUpdatedLevelNodes.find(pChildLevel) == m_setUpdatedLevelNodes.end() )
            pParentLevel->RemoveChildLevel(pChildLevel);
    }
}
