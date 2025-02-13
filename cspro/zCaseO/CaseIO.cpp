#include "stdafx.h"
#include "Pre74_Case.h"
#include <zToolsO/NewlineSubstitutor.h>


namespace CaseIOConstants
{
    constexpr TCHAR ErasedRecordCharacter = '~';
    constexpr TCHAR BlankCharacter        = ' ';
};

namespace Constants = CaseIOConstants;



void Pre74_Case::CalculateConstructionVariables()
{
    // initialize the variables
    m_iRecTypeLen = m_pDict->GetRecTypeLen();
    m_iRecTypeStart = m_pDict->GetRecTypeStart() - 1;
    m_aRecTypes = NULL;
    m_iNumRecTypes = 0;
    m_iLastRecTypePtr = 0;
    m_iMinLineLen = 0;
    m_iNumLevels = m_pDict->GetNumLevels();
    m_aLevelKeys = NULL;
    m_iLineLen = 0;
    m_pszLine = NULL;
    m_iLevel = 0;
    m_iRecord = 0;

    // process the record types
    for( int iPass = 1; iPass <= 2; iPass++ )
    {
        if( iPass == 2 )
        {
            m_aRecTypes = new RecordTypes[m_iNumRecTypes];
            m_iNumRecTypes = 0;
        }

        for( int iLevel = 0; iLevel < m_iNumLevels; iLevel++ )
        {
            const DictLevel& dict_level = m_pDict->GetLevel(iLevel);

            for( int iRecord = 0; iRecord < dict_level.GetNumRecords(); iRecord++ )
            {
                if( iPass == 2 )
                {
                    const CDictRecord* pRecord = dict_level.GetRecord(iRecord);

                    m_aRecTypes[m_iNumRecTypes].Type = new TCHAR[m_iRecTypeLen];
                    _tmemset(m_aRecTypes[m_iNumRecTypes].Type,Constants::BlankCharacter,m_iRecTypeLen);
                    _tmemcpy(m_aRecTypes[m_iNumRecTypes].Type,pRecord->GetRecTypeVal(),std::min(pRecord->GetRecTypeVal().GetLength(),m_iRecTypeLen));

                    m_aRecTypes[m_iNumRecTypes].Level = iLevel;
                    m_aRecTypes[m_iNumRecTypes].Record = iRecord;
                }

                m_iNumRecTypes++;
            }
        }
    }


    // calculate the minimum valid line length and information about each level's keys
    if( m_iRecTypeLen != 0 )
        m_iMinLineLen = m_iRecTypeStart + m_iRecTypeLen;

    m_aLevelKeys = new LevelKeys[m_iNumLevels];

    for( int iLevel = 0; iLevel < m_iNumLevels; iLevel++ )
    {
        m_aLevelKeys[iLevel].NumKeys = 0;
        m_aLevelKeys[iLevel].KeyLen = 0;

        for( int iPass = 1; iPass <= 2; iPass++ )
        {
            if( iPass == 2 )
            {
                m_aLevelKeys[iLevel].Keys = new LevelKeys::LevelKeysItem[m_aLevelKeys[iLevel].NumKeys];
                m_aLevelKeys[iLevel].NumKeys = 0;
            }

            // store information about all the keys up to and including the current level
            int iPrevEndPos = -1;

            for( int iThisLevel = 0; iThisLevel <= iLevel; iThisLevel++ )
            {
                const DictLevel& this_dict_level = m_pDict->GetLevel(iThisLevel);
                const CDictRecord* pRecord = this_dict_level.GetIdItemsRec();

                for( int iItem = 0; iItem < pRecord->GetNumItems(); iItem++ )
                {
                    const CDictItem* pItem = pRecord->GetItem(iItem);

                    if( iItem == 0 || (int)pItem->GetStart() != iPrevEndPos )
                    {
                        if( iPass == 2 )
                        {
                            m_aLevelKeys[iLevel].Keys[m_aLevelKeys[iLevel].NumKeys].Start = pItem->GetStart() - 1;
                            m_aLevelKeys[iLevel].Keys[m_aLevelKeys[iLevel].NumKeys].Length = pItem->GetLen();
                        }

                        m_aLevelKeys[iLevel].NumKeys++;
                    }

                    else if( iPass == 2 )
                    {
                        m_aLevelKeys[iLevel].Keys[m_aLevelKeys[iLevel].NumKeys - 1].Length += pItem->GetLen();
                    }

                    iPrevEndPos = pItem->GetStart() + pItem->GetLen();

                    if( iPass == 2 )
                    {
                        m_aLevelKeys[iLevel].KeyLen += pItem->GetLen();
                        m_iMinLineLen = (int)std::max((UINT)m_iMinLineLen,pItem->GetStart() + pItem->GetLen() - 1);
                    }
                }
            }
        }
    }
}


void Pre74_Case::DestructConstructionVariables()
{
    if( m_aRecTypes != NULL )
    {
        for( int i = 0; i < m_iNumRecTypes; i++ )
            safe_delete_array(m_aRecTypes[i].Type);

        safe_delete_array(m_aRecTypes);
    }

    for( int i = 0; i < m_iNumLevels; i++ )
        safe_delete_array(m_aLevelKeys[i].Keys);

    safe_delete_array(m_aLevelKeys);
}


bool Pre74_Case::GetRecordType()
{
    // if there are no record types, m_iLevel and m_iRecord don't need to be changed from their default defaults
    if( m_iRecTypeLen == 0 )
        return true;

    const TCHAR* m_pszRecType = m_pszLine + m_iRecTypeStart;

    // generally each line read will have a record type equal to or occurring after the previously read one, so
    // instead of searching from the beginning of the list each time, we will start from where we left off previously
    for( int iPass = 1; iPass <= 2; iPass++ )
    {
        int iRecType = ( iPass == 1 ) ? m_iLastRecTypePtr : 0;
        int iEnd = ( iPass == 1 ) ? m_iNumRecTypes : m_iLastRecTypePtr;

        for( ; iRecType < iEnd; iRecType++ )
        {
            bool bMatch = false;

            if( m_iRecTypeLen == 1 ) // speed up the check for the most common record type length
                bMatch = ( m_pszRecType[0] == m_aRecTypes[iRecType].Type[0] );

            else
                bMatch = ( _tmemcmp(m_pszRecType,m_aRecTypes[iRecType].Type,m_iRecTypeLen) == 0 );

            if( bMatch )
            {
                m_iLevel = m_aRecTypes[iRecType].Level;
                m_iRecord = m_aRecTypes[iRecType].Record;
                m_iLastRecTypePtr = iRecType;
                return true;
            }
        }
    }

    return false;
}


void Pre74_Case::GenerateKey(CString& csKey,const TCHAR* pszLine,int iLevel)
{
    TCHAR* pszKeyBuffer = csKey.GetBufferSetLength(m_aLevelKeys[iLevel].KeyLen);

    for( int i = 0; i < m_aLevelKeys[iLevel].NumKeys; i++ )
    {
        _tmemcpy(pszKeyBuffer,pszLine + m_aLevelKeys[iLevel].Keys[i].Start,m_aLevelKeys[iLevel].Keys[i].Length);
        pszKeyBuffer += m_aLevelKeys[iLevel].Keys[i].Length;
    }

    csKey.ReleaseBuffer();
}


void Pre74_Case::CopyKeyToBuffer(TCHAR* pszOutputBuffer,TCHAR* pszInputBuffer,int iLevel)
{
    for( int i = 0; i < m_aLevelKeys[iLevel].NumKeys; i++ )
        _tmemcpy(pszOutputBuffer + m_aLevelKeys[iLevel].Keys[i].Start,pszInputBuffer + m_aLevelKeys[iLevel].Keys[i].Start,m_aLevelKeys[iLevel].Keys[i].Length);
}


void Pre74_Case::ApplySpecialOutputKey(const Pre74_Case* pInputCase, Pre74_CaseLevel* pSpecialOutputCaseLevel, CString csInputKey)
{
    // in special output dictionaries, the original input key is copied directly to the output key;
    // if the input key is larger than the output key, then the left part of the input key is truncated;
    // if the output key is larger than the input key, then the left part of the output key is space filled

    // transform the input key to an output key
    CString csOutputKey;
    int iInputKeyLengthAccountedFor = 0;

    for( int iLevel = 0; iLevel < pSpecialOutputCaseLevel->GetLevelNum(); iLevel++ )
    {
        const LevelKeys* pInputLevelKeys = ( iLevel < pInputCase->m_iNumLevels ) ? ( pInputCase->m_aLevelKeys + iLevel ) : nullptr;
        const LevelKeys* pOutputLevelKeys = m_aLevelKeys + iLevel;

        CIMSAString csThisLevelKey;

        if( pInputLevelKeys != nullptr )
        {
            csThisLevelKey = csInputKey.Mid(iInputKeyLengthAccountedFor, pInputLevelKeys->KeyLen);
            iInputKeyLengthAccountedFor += pInputLevelKeys->KeyLen;
        }

        int iLeftSpacesNeeded = pOutputLevelKeys->KeyLen - csOutputKey.GetLength() - csThisLevelKey.GetLength();

        if( iLeftSpacesNeeded <= 0 )
            csOutputKey.AppendFormat(_T("%s"), (LPCTSTR)csThisLevelKey - iLeftSpacesNeeded);

        else
            csOutputKey.AppendFormat(_T("%s%s"), (LPCTSTR)CString(_T(' '), iLeftSpacesNeeded), (LPCTSTR)csThisLevelKey);
    }

    const LevelKeys* pOutputLevelKeys = m_aLevelKeys + ( pSpecialOutputCaseLevel->GetLevelNum() - 1 );
    ASSERT(csOutputKey.GetLength() == pOutputLevelKeys->KeyLen);

    // copy the key to this level and then any children levels
    std::function<void(Pre74_CaseLevel*)> key_copier =
        [this, pSpecialOutputCaseLevel, pOutputLevelKeys, &csOutputKey, &key_copier](Pre74_CaseLevel* pParentLevel)
    {
        if( pParentLevel == nullptr )
            return;

        const DictLevel& dict_level = pParentLevel->GetDictLevel();
        bool bGeneratedKey = false;

        for( int iRecord = 0; iRecord < dict_level.GetNumRecords(); iRecord++ )
        {
            Pre74_CaseRecord* pCaseRecord = pParentLevel->GetRecord(iRecord);

            for( int iOcc = 0; iOcc < pCaseRecord->GetNumRecordOccs(); iOcc++ )
            {
                const TCHAR* pInputKeyBuffer = csOutputKey.GetBuffer() + csOutputKey.GetLength();
                TCHAR* pRecordBuffer = pCaseRecord->GetOrCreateRecordBuffer(iOcc);

                for( int i = pOutputLevelKeys->NumKeys - 1; i >= 0; i-- )
                {
                    TCHAR* pKeyBuffer = pRecordBuffer + pOutputLevelKeys->Keys[i].Start;
                    pInputKeyBuffer -= pOutputLevelKeys->Keys[i].Length;
                    _tmemcpy(pKeyBuffer, pInputKeyBuffer, pOutputLevelKeys->Keys[i].Length);
                }

                if( !bGeneratedKey )
                {
                    GenerateKey(pParentLevel->m_csKey, pRecordBuffer, pParentLevel->GetLevelNum() - 1);
                    bGeneratedKey = true;
                }
            }
        }

        for( int iChildLevel = 0; iChildLevel < pParentLevel->GetNumChildLevels(); iChildLevel++ )
            key_copier(pParentLevel->GetChildLevel(iChildLevel));
    };

    key_copier(pSpecialOutputCaseLevel);
}


void Pre74_Case::Construct(std::vector<CString> case_lines)
{
    Reset();

    CString csTempLine; // for constructing a space-filled string if the input line was not long enough

    Pre74_CaseLevel* pLastLevelRead = NULL;
    Pre74_CaseLevel* pLastSecondLevelRead = NULL;
    bool bValidFirstLevelRecordRead = false;

    for( CString& csLine : case_lines )
    {
        m_iLineLen = csLine.GetLength();

        // turn ␤ -> \n
        NewlineSubstitutor::MakeUnicodeNLToNewline(csLine);

        m_pszLine = csLine.GetString();

        // ensure that the line is long enough for all of the IDs and the record type
        if( m_iLineLen < m_iMinLineLen )
        {
            csTempLine = csLine + CString(Constants::BlankCharacter,m_iMinLineLen - m_iLineLen);
            m_iLineLen = csTempLine.GetLength();
            m_pszLine = csTempLine.GetString();
        }

        // is the record erased?
        if( m_pszLine[0] == Constants::ErasedRecordCharacter )
            continue;

        // set the first level case ID (if necessary)
        if( pLastLevelRead == NULL )
        {
            pLastLevelRead = m_pRootLevel;
            GenerateKey(pLastLevelRead->m_csKey,m_pszLine,0);
        }

        // is the record type invalid?
        if( !GetRecordType() )
            continue;

        if( m_iLevel == 0 ) // on the first level
        {
#ifdef _DEBUG
            CString csThisKey;
            GenerateKey(csThisKey,m_pszLine,m_iLevel);
            ASSERT(csThisKey.CompareNoCase(pLastLevelRead->m_csKey) == 0);
#endif
            bValidFirstLevelRecordRead = true;

            if( pLastLevelRead != m_pRootLevel )
                continue;
        }

        else // on the second or third level
        {
            if( !bValidFirstLevelRecordRead )
                continue;

            CString csThisKey;
            GenerateKey(csThisKey,m_pszLine,m_iLevel);

            if( csThisKey.CompareNoCase(pLastLevelRead->m_csKey) != 0 )
            {
                // this is a new level or a new node

                if( m_iLevel == 1 )
                {
                    // this is a new second-level node
                    pLastLevelRead = m_pRootLevel->AddChildLevel(m_pDict->GetLevel(m_iLevel));
                    pLastSecondLevelRead = pLastLevelRead;
                }

                else
                {
                    ASSERT(m_iLevel == 2);

                    CString csSecondLevelKey;
                    GenerateKey(csSecondLevelKey,m_pszLine,1);

                    if( pLastSecondLevelRead != NULL && csSecondLevelKey.CompareNoCase(pLastSecondLevelRead->m_csKey) == 0 )
                    {
                        // this is a new node under the parent second-level node
                        pLastLevelRead = pLastSecondLevelRead->AddChildLevel(m_pDict->GetLevel(m_iLevel));
                    }

                    else
                    {
                        // this may not happen, but it would be a case where a third-level node occurred without
                        // a corresponding second-level node existing

                        // add the (empty) second-level node
                        pLastSecondLevelRead = m_pRootLevel->AddChildLevel(m_pDict->GetLevel(m_iLevel - 1));
                        FinalizeLevel(pLastSecondLevelRead,true,false,nullptr); // add the required records
                        ApplySpecialOutputKey(this, pLastSecondLevelRead, csSecondLevelKey); // and apply the key

                        // add the third-level node
                        pLastLevelRead = pLastSecondLevelRead->AddChildLevel(m_pDict->GetLevel(m_iLevel));
                    }
                }

                pLastLevelRead->m_csKey = csThisKey;
            }
        }

        Pre74_CaseRecord* pCaseRecord = pLastLevelRead->m_apRecords[m_iRecord];

        if( pCaseRecord->m_iNumOccs == pCaseRecord->m_iMaxRecs )
            continue;

        TCHAR* pszRecordBuffer = pCaseRecord->m_apszRecordStorage[pCaseRecord->m_iNumOccs];

        // allocate memory for the record (if needed)
        if( pszRecordBuffer == NULL )
        {
            pCaseRecord->m_apszRecordStorage[pCaseRecord->m_iNumOccs] = new TCHAR[pCaseRecord->m_iRecLength];
            pszRecordBuffer = pCaseRecord->m_apszRecordStorage[pCaseRecord->m_iNumOccs];
        }

        // copy the record
        int iCharsToCopy = std::min(pCaseRecord->m_iRecLength,m_iLineLen);
        int iSpacesToFill = pCaseRecord->m_iRecLength - iCharsToCopy;

        _tmemcpy(pszRecordBuffer,m_pszLine,iCharsToCopy);

        if( iSpacesToFill != 0 )
            _tmemset(pszRecordBuffer + iCharsToCopy,Constants::BlankCharacter,iSpacesToFill);

        pCaseRecord->m_iNumOccs++;
    }
}
