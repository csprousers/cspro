#pragma once
//---------------------------------------------------------------------------
//  File name: DicX.h
//
//  Description:
//          Header for engine-Dic class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              10 Jul 00   vc      Full remake
//
//
//---------------------------------------------------------------------------
#include <engine/Tables.h>
#include <zUtilO/ConnectionString.h>
#include <zCaseO/CaseKey.h>
#include <zCaseO/Note.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryDefines.h>

class Case;
class CaseLevel;
class Pre74_CaseLevel;

class CEngineDriver;
class CaseIterator;
class DataRepository;
class CRelations;

//---------------------------------------------------------------------------
//
//  struct DICX
//
//  Description:
//      Parallel Dictionaries' table management
//
//  Construction/Destruction/Initialization
//      Init                        Initialization
//
//  Related objects
//      GetDicT                     Return the pointer to the DICT parallel to this DICX
//      SetDicT                     Set the pointer to the DICT parallel to this DICX
//
//  Basic data
//      GetMaxRecLength             Return the length of the record area for I/O
//      SetMaxRecLength             Set the length of the record area for I/O
//      SetKeyHolderLength          Set the key-holders' length
//      GetKeyHolderLength          Return key-holders' length
//
//  Data management
//      GetRecordArea               Return the address of the record area for I/O
//      SetRecordArea               Set the address of the record area for I/O
//
//  Engine links
//      SetEngineDriver
//
//---------------------------------------------------------------------------

struct NoteByCaseLevel
{
    size_t note_index;
    const Pre74_CaseLevel* pre74_case_level;
};


struct DICX {                           // DICX: execution Dict. table

// --- Data members --------------------------------------------------------
    // --- related objects
private:
    DICT*   m_pDicT;                    // DICT* brother

    // --- basic data
private:
    int     m_iMaxRecLength;            // Dict record length (max record size)
    int     m_iKeyHolderLen;            // key-holders' length

    // --- related items in the Dictionary
public:
    CRelations* pRelations;                  // RHF 12/8/99 Related items

    // --- data management
private:
    csprochar*   m_pRecArea;                 // record area for I/O of one record // formerly 'recad'
public:
    csprochar*   m_pKeyHolderArea;           // key-holders' area (for 2 keys)
    csprochar*   current_key;                // key-holder for current case
    csprochar*   last_key;                   // key-holder for last case

    int     LastOpenLevel;              // Last Level opened    // RHF 22/9/98
    int     level;                      // current node level
    int     nextlevel;                  // next node key level
    int     lastlevel;                  // last node key level

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;

    // legacy data - perhaps obsolete, check it         // victor Jul 10, 00
public:
    bool    entermode;                  // 1 => in data entry  0 => other (external dict)

// --- Methods -------------------------------------------------------------
    // --- initialization
public:
    void    Init();

    // --- related objects
public:
    const DICT* GetDicT() const { return m_pDicT; }
    DICT* GetDicT()             { return m_pDicT; }
    void SetDicT(DICT* pDicT)   { m_pDicT = pDicT; }

    // --- basic data
public:
    int     GetMaxRecLength() const        { return m_iMaxRecLength; } // victor Jul 13, 00
    void    SetMaxRecLength( int iLen )    { m_iMaxRecLength = iLen; } // victor Jul 13, 00
    int     GetKeyHolderLength() const     { return m_iKeyHolderLen; } // victor Jul 13, 00
    void    SetKeyHolderLength( int iLen ) { m_iKeyHolderLen = iLen; } // victor Jul 13, 00

    // --- data management
public:
    csprochar* GetRecordArea()                 { return m_pRecArea; }      // victor Jul 13, 00
    void    SetRecordArea( csprochar* pArea )  { m_pRecArea = pArea; }     // victor Jul 13, 00

    // --- engine links
public:
    void    SetEngineDriver( CEngineDriver* pEngineDriver ) { m_pEngineDriver = pEngineDriver; }

public:
    void SetDataRepository(std::shared_ptr<DataRepository> data_repository);

    bool IsDataRepositoryOpen() const               { return ( m_dataRepository != nullptr ); }
    const DataRepository& GetDataRepository() const { ASSERT(IsDataRepositoryOpen()); return *m_dataRepository; }
    DataRepository& GetDataRepository()             { ASSERT(IsDataRepositoryOpen()); return *m_dataRepository; }

    // CR_TODO review above; below is good
public:
    void StartRuntime();
    void StopRuntime();

    void CloseDataRepository();

    const Case& GetCase() const { return *m_rd->m_case; }
    Case& GetCase()             { return *m_rd->m_case; }

    const ConnectionString& GetLastClosedConnectionString() const { return m_rd->m_lastClosedConnectionString; }

    bool IsLastSearchedCaseKeyDefined() const                                    { return m_rd->m_lastSearchedCaseKey.has_value(); }
    const std::optional<CaseKey>& GetLastSearchedCaseKey() const                 { return m_rd->m_lastSearchedCaseKey; }
    void ClearLastSearchedKey()                                                  { m_rd->m_lastSearchedCaseKey.reset(); }
    void SetLastSearchedCaseKey(const std::optional<CaseKey>& optional_case_key) { m_rd->m_lastSearchedCaseKey = optional_case_key; }

    CaseIterationMethod GetCaseIterationMethod() const                     { return m_rd->m_caseIterationMethod; }
    void SetCaseIterationMethod(CaseIterationMethod case_iteration_method) { m_rd->m_caseIterationMethod = case_iteration_method; }
    CaseIterationOrder GetCaseIterationOrder() const                       { return m_rd->m_caseIterationOrder; }
    void SetCaseIterationOrder(CaseIterationOrder case_iteration_order)    { m_rd->m_caseIterationOrder = case_iteration_order; }
    CaseIterationCaseStatus GetCaseIterationCaseStatus() const             { return m_rd->m_caseIterationCaseStatus; }
    void SetCaseIterationCaseStatus(CaseIterationCaseStatus case_status)   { m_rd->m_caseIterationCaseStatus = case_status; }
    std::tuple<CaseIterationMethod, CaseIterationOrder, CaseIterationCaseStatus> GetDictionaryAccessParameters(int dictionary_access) const;

    enum class CaseIteratorStyle { FromBoundary, FromNextKey, FromLastSearchedCaseKey, FromCurrentPosition };
    void CreateCaseIterator(CaseIteratorStyle case_iterator_style, std::optional<CaseKey> starting_key = std::nullopt,
                            int dictionary_access = 0, std::optional<CString> key_prefix = std::nullopt,
                            CaseIterationContent iteration_content = CaseIterationContent::Case);

    bool IsCaseIteratorActive() const           { return ( m_rd->m_caseIterator != nullptr ); }
    bool StepCaseIterator()                     { ASSERT(IsCaseIteratorActive()); return m_rd->m_caseIterator->NextCasetainer(*m_rd->m_case); }
    bool StepCaseKeyIterator(CaseKey& case_key) { ASSERT(IsCaseIteratorActive()); return m_rd->m_caseIterator->NextCaseKey(case_key); }
    int GetCaseIteratorPercentRead() const      { ASSERT(IsCaseIteratorActive()); return m_rd->m_caseIterator->GetPercentRead(); }
    void StopCaseIterator();

    const std::vector<NoteByCaseLevel>& GetNotesByCaseLevel() const { return m_rd->m_notesByCaseLevel; }
    std::vector<NoteByCaseLevel>& GetNotesByCaseLevel()             { return m_rd->m_notesByCaseLevel; }

    void ResetCaseObjects();

private:
    std::shared_ptr<DataRepository> m_dataRepository;

    struct RuntimeData
    {
        std::shared_ptr<Case> m_case;

        // this stores the connection string for the last closed repository
        ConnectionString m_lastClosedConnectionString;

        // this is set when find / locate is successfully executed
        std::optional<CaseKey> m_lastSearchedCaseKey;

	    // case iterators
        CaseIterationMethod m_caseIterationMethod;
        CaseIterationOrder m_caseIterationOrder;
        CaseIterationCaseStatus m_caseIterationCaseStatus;
        std::unique_ptr<CaseIterator> m_caseIterator;

        // notes by case level
        std::vector<NoteByCaseLevel> m_notesByCaseLevel;
    };

    RuntimeData* m_rd;
};
