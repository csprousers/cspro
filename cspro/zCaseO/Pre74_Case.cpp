#include "stdafx.h"
#include "Pre74_Case.h"
#include <zToolsO/NewlineSubstitutor.h>


Pre74_CaseRecord::Pre74_CaseRecord(const CDictRecord* pRecord)
    :   m_pDictRecord(pRecord),
        m_iNumOccs(0),
        m_iMaxRecs(pRecord->GetMaxRecs()),
        m_iRecLength(pRecord->GetRecLen())
{
    m_apszRecordStorage = new TCHAR*[m_iMaxRecs];
    memset(m_apszRecordStorage,0,m_iMaxRecs * sizeof(TCHAR*));
}


Pre74_CaseRecord::~Pre74_CaseRecord()
{
    // the record storage pointers will be NULL unless memory has been allocated for that
    // occurrence, so we can stop deleting memory when we reach a NULL pointer
    for( int i = 0; i < m_iMaxRecs && m_apszRecordStorage[i] != NULL; i++ )
        safe_delete_array(m_apszRecordStorage[i]);

    safe_delete_array(m_apszRecordStorage);
}


void Pre74_CaseRecord::Reset()
{
    m_iNumOccs = 0;
}


TCHAR* Pre74_CaseRecord::GetOrCreateRecordBuffer(int iOcc)
{
    ASSERT(iOcc < m_iMaxRecs);

    if( iOcc >= m_iNumOccs )
    {
        // create the new occurrence(s)
        for( ; m_iNumOccs <= iOcc; m_iNumOccs++ )
        {
            // only allocate record storage if not already allocated by an earlier case; this way we
            // can avoid a lot of reallocations when working with multiple cases (such as in batch)
            if( m_apszRecordStorage[m_iNumOccs] == NULL )
                m_apszRecordStorage[m_iNumOccs] = new TCHAR[m_iRecLength];

            _tmemset(m_apszRecordStorage[m_iNumOccs],_T(' '),m_iRecLength);
        }
    }

    return m_apszRecordStorage[iOcc];
}


Pre74_CaseLevel::Pre74_CaseLevel(const DictLevel& dict_level,int iLevelNum)
    :   m_dictLevel(dict_level),
        m_iLevelNum(iLevelNum),
        m_iNumRecords(dict_level.GetNumRecords()),
        m_apChildLevels(NULL),
        m_iNumChildLevels(0),
        m_iAllocatedChildLevels(0)
{
    // allocate the records
    m_apRecords = new Pre74_CaseRecord*[m_iNumRecords];

    for( int i = 0; i < m_iNumRecords; i++ )
        m_apRecords[i] = new Pre74_CaseRecord(dict_level.GetRecord(i));
}


Pre74_CaseLevel::~Pre74_CaseLevel()
{
    // delete the records
    for( int i = 0; i < m_iNumRecords; i++ )
        delete m_apRecords[i];

    safe_delete_array(m_apRecords);

    // delete any children levels
    for( int i = 0; i < m_iAllocatedChildLevels; i++ )
        delete m_apChildLevels[i];

    safe_delete_array(m_apChildLevels);
}


void Pre74_CaseLevel::Reset(bool bResetNumberChildLevels/*= true*/)
{
    m_csKey.Empty();

    for( int i = 0; i < m_iNumRecords; i++ )
        m_apRecords[i]->Reset();

    if( bResetNumberChildLevels )
        m_iNumChildLevels = 0;

    m_binaryStorageFor80.reset();
}


Pre74_CaseLevel* Pre74_CaseLevel::AddChildLevel(const DictLevel& dict_level)
{
    if( m_iNumChildLevels < m_iAllocatedChildLevels )
    {
        // reuse an already allocated level
        Pre74_CaseLevel* pChildLevel = m_apChildLevels[m_iNumChildLevels];
        pChildLevel->Reset();
        m_iNumChildLevels++;
        return pChildLevel;
    }

    // reallocate memory for the levels
    Pre74_CaseLevel** apNewChildLevels = new Pre74_CaseLevel*[m_iAllocatedChildLevels + 1];

    if( m_iAllocatedChildLevels > 0 )
    {
        // copy over the previous level pointers
        for( int i = 0; i < m_iAllocatedChildLevels; i++ )
            apNewChildLevels[i] = m_apChildLevels[i];

        // delete the old array
        safe_delete_array(m_apChildLevels);
    }

    m_apChildLevels = apNewChildLevels;

    // create the new level
    Pre74_CaseLevel* pChildLevel = new Pre74_CaseLevel(dict_level, m_iLevelNum + 1);

    m_apChildLevels[m_iNumChildLevels] = pChildLevel;

    m_iNumChildLevels++;
    m_iAllocatedChildLevels++;

    return pChildLevel;
}


Pre74_CaseLevel* Pre74_CaseLevel::InsertChildLevel(const Pre74_CaseLevel* pCaseLevelInsertBefore)
{
    ASSERT(( m_iLevelNum + 1 ) == pCaseLevelInsertBefore->m_iLevelNum);

    // first add a new child level (to allocate space for this new level)
    Pre74_CaseLevel* pCaseLevelAdded = AddChildLevel(pCaseLevelInsertBefore->m_dictLevel);

    // set the key to the parent's key, to be modified later
    pCaseLevelAdded->m_csKey = m_csKey;

    // now reorder the levels so that the newly added level comes before pCaseLevelInsertBefore
    for( int i = ( m_iNumChildLevels - 1 ); ; i-- )
    {
        m_apChildLevels[i] = m_apChildLevels[i - 1];

        if( m_apChildLevels[i] == pCaseLevelInsertBefore )
        {
            m_apChildLevels[i - 1] = pCaseLevelAdded;
            break;
        }
    }

    return pCaseLevelAdded;
}

Pre74_CaseLevel* Pre74_CaseLevel::AddChildLevelAtEnd(const Pre74_CaseLevel* pCaseLevelAddAfter)
{
    // add a new child level
    Pre74_CaseLevel* pCaseLevelAdded = AddChildLevel(pCaseLevelAddAfter->m_dictLevel);

    // set the key to the parent's key, to be modified later
    pCaseLevelAdded->m_csKey = m_csKey;

    return pCaseLevelAdded;
}


CString Pre74_CaseLevel::GetKey() const
{
    return m_csKey;
}


Pre74_CaseRecord* Pre74_CaseLevel::GetRecord(int iRecNum)
{
    ASSERT(iRecNum < m_iNumRecords);
    return m_apRecords[iRecNum];
}

Pre74_CaseRecord* Pre74_CaseLevel::GetRecord(const CDictRecord* pDictRecord)
{
    // REPO_TEMP: check where this is used and see if this can be optimized; for example, perhaps the section
    // objects can store, instead of a pointer to the dictionary record, an integer of the record index

    for( int i = 0; i < m_iNumRecords; i++ )
    {
        if( m_apRecords[i]->GetDictRecord() == pDictRecord )
            return m_apRecords[i];
    }

    return NULL;
}


const TCHAR* Pre74_CaseLevel::GetFirstFilledRecordBuffer()
{
    for( int i = 0; i < m_iNumRecords; i++ )
    {
        if( m_apRecords[i]->GetNumRecordOccs() > 0 )
            return m_apRecords[i]->GetRecordBuffer(0);
    }

    return NULL;
}


void Pre74_CaseLevel::RemoveChildLevel(Pre74_CaseLevel* pCaseLevel)
{
    for( int i = 0; i < m_iNumChildLevels; i++ )
    {
        if( m_apChildLevels[i] == pCaseLevel )
        {
            // shift all of the subsequent levels
            for( int j = i + 1; j < m_iNumChildLevels; j++ )
                m_apChildLevels[j - 1] = m_apChildLevels[j];

            m_iNumChildLevels--;

            // store the old level at the new end so that it can be reused
            m_apChildLevels[m_iNumChildLevels] = pCaseLevel;

            return;
        }
    }

    ASSERT(false);
}


Pre74_Case::Pre74_Case(const CDataDict* pDict)
    :
    m_pDict(pDict)
{
    CalculateConstructionVariables();
    m_pRootLevel = new Pre74_CaseLevel(pDict->GetLevel(0),1);
}


Pre74_Case::~Pre74_Case()
{
    safe_delete(m_pRootLevel);
    DestructConstructionVariables();
}


void Pre74_Case::Reset()
{
    m_pRootLevel->Reset();

    m_iLastRecTypePtr = 0;
}


void Pre74_Case::FinalizeLevel(Pre74_CaseLevel* pCaseLevel,bool bAddRequiredRecords,bool bThrowCaseHasNoValidRecordsException,
    CaseConstructionReporter* case_construction_reporter)
{
    TCHAR* pszFirstValidRecordBuffer = NULL;

    const DictLevel& dict_level = pCaseLevel->GetDictLevel();

    std::vector<TCHAR*> apRequiredRecordsWithoutOccurrences;

    for( int iRecType = 0; iRecType < dict_level.GetNumRecords(); iRecType++ )
    {
        const CDictRecord* pDictRecord = dict_level.GetRecord(iRecType);
        Pre74_CaseRecord* pCaseRecord = pCaseLevel->GetRecord(iRecType);

        bool bIsRequiredRecordWithoutData = false;

        // create a record if necessary
        if( bAddRequiredRecords && pDictRecord->GetRequired() && pCaseRecord->GetNumRecordOccs() == 0 )
        {
            bIsRequiredRecordWithoutData = true;
            apRequiredRecordsWithoutOccurrences.emplace_back(pCaseRecord->GetOrCreateRecordBuffer(0));

            if( case_construction_reporter != nullptr )
                case_construction_reporter->BlankRecordAdded(pCaseLevel->m_csKey, pDictRecord->GetName());
        }

        for( int iOcc = 0; iOcc < pCaseRecord->GetNumRecordOccs(); iOcc++ )
        {
            TCHAR* pszRecordBuffer = pCaseRecord->GetRecordBuffer(iOcc);

            // generate the key
            if( pszFirstValidRecordBuffer == NULL && !bIsRequiredRecordWithoutData )
            {
                pszFirstValidRecordBuffer = pszRecordBuffer;
                GenerateKey(pCaseLevel->m_csKey,pszFirstValidRecordBuffer,pCaseLevel->GetLevelNum() - 1);
            }

            // copy over the record type
            if( m_iRecTypeLen > 0 )
                _tmemcpy(pszRecordBuffer + m_iRecTypeStart,pDictRecord->GetRecTypeVal(),m_iRecTypeLen);
        }
    }

    if( pszFirstValidRecordBuffer != NULL )
    {
        // add occurrences for any required records without data
        for( TCHAR* apRequiredRecordsWithoutOccurrence : apRequiredRecordsWithoutOccurrences )
            CopyKeyToBuffer(apRequiredRecordsWithoutOccurrence, pszFirstValidRecordBuffer, pCaseLevel->GetLevelNum() - 1);
    }

    else if( bAddRequiredRecords )
    {
        // ensure that there is at least one record in the case
        if( apRequiredRecordsWithoutOccurrences.empty() )
        {
            const CDictRecord* pFirstDictRecord = dict_level.GetRecord(0);
            Pre74_CaseRecord* pFirstCaseRecord = pCaseLevel->GetRecord(0);

            TCHAR* pszRecordBuffer = pFirstCaseRecord->GetOrCreateRecordBuffer(0);

            // copy over the record type
            if( m_iRecTypeLen > 0 )
                _tmemcpy(pszRecordBuffer + m_iRecTypeStart,pFirstDictRecord->GetRecTypeVal(),m_iRecTypeLen);
        }

        if( bThrowCaseHasNoValidRecordsException )
        {
            throw CaseHasNoValidRecordsException();
        }

        else
        {
            ASSERT(false); // this should rarely happen but can happen if:
                           // - a special output dictionary level has nothing assigned to it
        }
    }

    // ensure that the current key is copied to any children levels
    CopyKeyToChildren(pCaseLevel,pszFirstValidRecordBuffer,pCaseLevel->GetLevelNum() - 1);
}


void Pre74_Case::CopyKeyToChildren(Pre74_CaseLevel* pParentLevel,TCHAR* pszInputBuffer,int iKeyLevel)
{
    if( pParentLevel == nullptr )
        return;

    for( int iChildLevel = 0; iChildLevel < pParentLevel->GetNumChildLevels(); iChildLevel++ )
    {
        Pre74_CaseLevel* pChildLevel = pParentLevel->GetChildLevel(iChildLevel);
        const DictLevel& dict_level = pChildLevel->GetDictLevel();
        TCHAR* pszValidRecordBuffer = nullptr;

        for( int iRecType = 0; iRecType < dict_level.GetNumRecords(); iRecType++ )
        {
            Pre74_CaseRecord* pCaseRecord = pChildLevel->GetRecord(iRecType);

            for( int iOcc = 0; iOcc < pCaseRecord->GetNumRecordOccs(); iOcc++ )
            {
                pszValidRecordBuffer = pCaseRecord->GetRecordBuffer(iOcc);
                CopyKeyToBuffer(pszValidRecordBuffer,pszInputBuffer,iKeyLevel);
            }
        }

        // update the case level's key
        GenerateKey(pChildLevel->m_csKey,pszValidRecordBuffer,pChildLevel->GetLevelNum() - 1);

        CopyKeyToChildren(pChildLevel,pszInputBuffer,iKeyLevel);
    }
}


void Pre74_Case::GetCaseLines(std::vector<CString>& case_lines, Pre74_CaseLevel* pCaseLevel) const
{
    for( int iRecType = 0; iRecType < pCaseLevel->m_iNumRecords; iRecType++ )
    {
        Pre74_CaseRecord* pCaseRecord = pCaseLevel->GetRecord(iRecType);

        for( int iOcc = 0; iOcc < pCaseRecord->GetNumRecordOccs(); iOcc++ )
        {
            CString& csLine = case_lines.emplace_back(pCaseRecord->GetRecordBuffer(iOcc), pCaseRecord->m_iRecLength);

            // turn \n -> ␤
            NewlineSubstitutor::MakeNewlineToUnicodeNL(csLine);
            SO::MakeTrimRightSpace(csLine);
        }
    }

    for( int iChildLevel = 0; iChildLevel < pCaseLevel->GetNumChildLevels(); iChildLevel++ )
        GetCaseLines(case_lines, pCaseLevel->GetChildLevel(iChildLevel));
}


void Pre74_Case::GetLevelArray(std::vector<const Pre74_CaseLevel*>& case_levels, const Pre74_CaseLevel* pCaseLevel) const
{
    case_levels.emplace_back(pCaseLevel);

    for( int i = 0; i < pCaseLevel->GetNumChildLevels(); i++ )
        GetLevelArray(case_levels, pCaseLevel->GetChildLevel(i));
}

std::vector<const Pre74_CaseLevel*> Pre74_Case::GetLevelArray() const
{
    std::vector<const Pre74_CaseLevel*> case_levels;
    GetLevelArray(case_levels, m_pRootLevel);

    return case_levels;
}


Pre74_CaseLevel* Pre74_Case::GetLastLevel(int iLevelNum,Pre74_CaseLevel* pProcessingLevel/* = NULL*/)
{
    if( pProcessingLevel == NULL )
        pProcessingLevel = m_pRootLevel;

    if( pProcessingLevel->GetLevelNum() == iLevelNum )
        return pProcessingLevel;

    Pre74_CaseLevel* pLastLevel = NULL;

    for( int i = 0; i < pProcessingLevel->GetNumChildLevels(); i++ )
    {
        Pre74_CaseLevel* pCandidateLevel = GetLastLevel(iLevelNum,pProcessingLevel->GetChildLevel(i));

        if( pCandidateLevel != NULL )
            pLastLevel = pCandidateLevel;
    }

    return pLastLevel;
}


Pre74_CaseLevel* Pre74_Case::FindParentLevel(Pre74_CaseLevel* pCaseLevel)
{
    return FindParentLevel(pCaseLevel,m_pRootLevel);
}

Pre74_CaseLevel* Pre74_Case::FindParentLevel(Pre74_CaseLevel* pCaseLevel,Pre74_CaseLevel* pParentLevel)
{
    for( int i = 0; i < pParentLevel->GetNumChildLevels(); i++ )
    {
        Pre74_CaseLevel* pChildLevel = pParentLevel->GetChildLevel(i);

        if( pCaseLevel == pChildLevel )
            return pParentLevel;

        Pre74_CaseLevel* pFoundParentLevel = FindParentLevel(pCaseLevel,pChildLevel);

        if( pFoundParentLevel != nullptr )
            return pFoundParentLevel;
    }

    return nullptr;
}


CString Pre74_Case::GetLevelKey(const Pre74_CaseLevel* pCaseLevel, bool bIncludeAllNonRootLevels/* = true*/) const
{
    CString csLevelKey;

    if( m_pRootLevel != pCaseLevel )
    {
        int iPreviousLevelKeyLength = bIncludeAllNonRootLevels ? m_aLevelKeys[0].KeyLen : m_aLevelKeys[pCaseLevel->GetLevelNum() - 2].KeyLen;
        csLevelKey = pCaseLevel->GetKey().Mid(iPreviousLevelKeyLength);
    }

    return csLevelKey;
}


int Pre74_Case::GetNumberNodes() const
{
    return GetNumberNodes(m_pRootLevel);
}

int Pre74_Case::GetNumberNodes(Pre74_CaseLevel* pCaseLevel) const
{
    int iNumber = 1;

    for( int i = 0; i < pCaseLevel->GetNumChildLevels(); i++ )
        iNumber += GetNumberNodes(pCaseLevel->GetChildLevel(i));

    return iNumber;
}


int Pre74_Case::GetCaseLevelNodeNumber(Pre74_CaseLevel* pCaseLevel) const
{
    int iNumber = 0;
    return GetCaseLevelNodeNumber(pCaseLevel,m_pRootLevel,iNumber) ? iNumber : -1;
}

bool Pre74_Case::GetCaseLevelNodeNumber(Pre74_CaseLevel* pSearchCaseLevel,Pre74_CaseLevel* pCurrentCaseLevel,int& iNumber) const
{
    if( pSearchCaseLevel == pCurrentCaseLevel )
        return true;

    for( int i = 0; i < pCurrentCaseLevel->GetNumChildLevels(); i++ )
    {
        iNumber++;

        if( GetCaseLevelNodeNumber(pSearchCaseLevel,pCurrentCaseLevel->GetChildLevel(i),iNumber) )
            return true;
    };

    return false;
}


Pre74_CaseLevel* Pre74_Case::GetCaseLevelAtNodeNumber(int iNode)
{
    return GetCaseLevelAtNodeNumber(m_pRootLevel,iNode);
}

Pre74_CaseLevel* Pre74_Case::GetCaseLevelAtNodeNumber(Pre74_CaseLevel* pCurrentCaseLevel,int& iNode)
{
    Pre74_CaseLevel* pSearchCaseLevel = nullptr;

    if( iNode == 0 ) // we have reached the goal node
        pSearchCaseLevel = pCurrentCaseLevel;

    else
    {
        iNode--;

        for( int i = 0; pSearchCaseLevel == nullptr && i < pCurrentCaseLevel->GetNumChildLevels(); i++ )
            pSearchCaseLevel = GetCaseLevelAtNodeNumber(pCurrentCaseLevel->GetChildLevel(i),iNode);
    }

    return pSearchCaseLevel;
}


int Pre74_Case::CalculateRecordsForWriting() const
{
    return CalculateRecordsForWriting(m_pRootLevel);
}

int Pre74_Case::CalculateRecordsForWriting(Pre74_CaseLevel* pCaseLevel) const
{
    int iRecords = 0;

    // calculate the number of records in this level
    for( int i = 0; i < pCaseLevel->m_iNumRecords; i++ )
        iRecords += pCaseLevel->m_apRecords[i]->GetNumRecordOccs();

    // calculate the number of records in any children levels
    for( int i = 0; i < pCaseLevel->GetNumChildLevels(); i++ )
        iRecords += CalculateRecordsForWriting(pCaseLevel->GetChildLevel(i));

    return iRecords;
}
