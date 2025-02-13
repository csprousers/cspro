#pragma once

#include <zCaseO/zCaseO.h>

// REPO_TODO: document these classes
// I'm slowly adding functionality from the other CCase classes

struct BinaryStorageFor80;
class CDictRecord;
class CDataDict;
class CaseConstructionReporter;
class CaseHasNoValidRecordsException {};
class DictLevel;


class ZCASEO_API Pre74_CaseRecord
{
    friend class Pre74_Case;

public:
    Pre74_CaseRecord(const CDictRecord* pDictRecord);
    ~Pre74_CaseRecord();

    void Reset();

    const CDictRecord* GetDictRecord() { return m_pDictRecord; }

    int GetNumRecordOccs() const { return m_iNumOccs; }
    int GetRecordLength() const { return m_iRecLength; }

    TCHAR* GetRecordBuffer(int iOcc) { ASSERT(iOcc < m_iNumOccs); return m_apszRecordStorage[iOcc]; }
    TCHAR* GetOrCreateRecordBuffer(int iOcc);

private:
    TCHAR** m_apszRecordStorage;

    const CDictRecord* m_pDictRecord;
    int m_iNumOccs;
    int m_iMaxRecs;
    int m_iRecLength;
};


class ZCASEO_API Pre74_CaseLevel
{
    friend class Case;
    friend class Pre74_Case;

public:
    Pre74_CaseLevel(const DictLevel& dict_level, int iLevelNum);
    ~Pre74_CaseLevel();

    void Reset(bool bResetNumberChildLevels = true);

    const DictLevel& GetDictLevel() const { return m_dictLevel; }

    Pre74_CaseLevel* AddChildLevel(const DictLevel& dict_level);

    Pre74_CaseLevel* InsertChildLevel(const Pre74_CaseLevel* pCaseLevelInsertBefore);
    Pre74_CaseLevel* AddChildLevelAtEnd(const Pre74_CaseLevel* pCaseLevelAddAfter);

    CString GetKey() const;

    /// <summary>
    /// Get the 1-based number of the level (i.e., the root level is 1).
    /// </summary>
    int GetLevelNum() const { return m_iLevelNum; }

    Pre74_CaseRecord* GetRecord(int iRecNum);
    Pre74_CaseRecord* GetRecord(const CDictRecord* pDictRecord);
    const TCHAR* GetFirstFilledRecordBuffer();

    int GetNumChildLevels() const { return m_iNumChildLevels; }
    const Pre74_CaseLevel* GetChildLevel(int iLevelNum) const { return m_apChildLevels[iLevelNum]; }
    Pre74_CaseLevel* GetChildLevel(int iLevelNum) { return m_apChildLevels[iLevelNum]; }
    void RemoveChildLevel(Pre74_CaseLevel* pCaseLevel);

private:
    CString m_csKey;

    const DictLevel& m_dictLevel;
    int m_iLevelNum;

    Pre74_CaseRecord** m_apRecords;
    int m_iNumRecords;

    Pre74_CaseLevel** m_apChildLevels;
    int m_iNumChildLevels;
    int m_iAllocatedChildLevels;

public:
    std::unique_ptr<std::vector<std::shared_ptr<BinaryStorageFor80>>> m_binaryStorageFor80;
};


class ZCASEO_API Pre74_Case
{
public:
    Pre74_Case(const CDataDict* pDict);
    ~Pre74_Case();

    void Reset();

    /// <summary>
    /// REPO_TODO...add description
    /// </summary>
    void Construct(std::vector<CString> case_lines);

    /// <summary>
    /// REPO_TODO...add description -- all levels should be finalized before this has been called
    /// </summary>
    void GetCaseLines(std::vector<CString>& case_lines) const { GetCaseLines(case_lines, m_pRootLevel); }

    CString GetKey() const { return m_pRootLevel->GetKey(); }

    const Pre74_CaseLevel* GetRootLevel() const { return m_pRootLevel; }
    Pre74_CaseLevel* GetRootLevel() { return m_pRootLevel; }

    /// <summary>
    /// REPO_TODO...add description
    /// </summary>
    void GenerateKey(CString& csKey,const TCHAR* pszLine,int iLevel);

    /// <summary>
    /// REPO_TODO...add description ... used to copy a batch input's key onto the level
    /// </summary>
    void ApplySpecialOutputKey(const Pre74_Case* pInputCase, Pre74_CaseLevel* pSpecialOutputCaseLevel, CString csInputKey);

    /// <summary>
    /// Based on the records added to the level, generates a key for the level and adds the
    /// record types to the record buffers. If a record is required but does not exist, a record can
    /// optionally be created.
    /// </summary>
    void FinalizeLevel(Pre74_CaseLevel* pCaseLevel,bool bAddRequiredRecords,bool bThrowCaseHasNoValidRecordsException,
        CaseConstructionReporter* case_construction_reporter);

    /// <summary>
    /// Get a vector filled with pointers to all of the levels in the case.
    /// </summary>
    std::vector<const Pre74_CaseLevel*> GetLevelArray() const;

    /// <summary>
    /// Gets the last level at the given one-based level number or NULL if such a level does not exist.
    /// </summary>
    Pre74_CaseLevel* GetLastLevel(int iLevelNum,Pre74_CaseLevel* pProcessingLevel = NULL);

    /// <summary>
    /// Finds the parent of the passed-in level.
    /// </summary>
    Pre74_CaseLevel* FindParentLevel(Pre74_CaseLevel* pCaseLevel);

    /// <summary>
    /// Gets the level key, a key not including the first level ID items, of a level.
    /// </summary>
    CString GetLevelKey(const Pre74_CaseLevel* pCaseLevel, bool bIncludeAllNonRootLevels = true) const;

    /// <summary>
    /// Gets the total number of level nodes in the case.
    /// </summary>
    int GetNumberNodes() const;

    /// <summary>
    /// Gets the node number for a given level or -1 if not found.
    int GetCaseLevelNodeNumber(Pre74_CaseLevel* pSearchCaseLevel) const;
    /// </summary>

    /// <summary>
    /// Gets the level at the given node number or nullptr if not found.
    Pre74_CaseLevel* GetCaseLevelAtNodeNumber(int iNode);
    /// </summary>

    /// <summary>
    int CalculateRecordsForWriting() const;
    /// </summary>

private:
    void CalculateConstructionVariables();
    void DestructConstructionVariables();

    bool GetRecordType();
    void CopyKeyToBuffer(TCHAR* pszOutputBuffer,TCHAR* pszInputBuffer,int iLevel);
    void CopyKeyToChildren(Pre74_CaseLevel* pParentLevel,TCHAR* pszInputBuffer,int iKeyLevel);

    void GetLevelArray(std::vector<const Pre74_CaseLevel*>& case_levels, const Pre74_CaseLevel* pCaseLevel) const;

    void GetCaseLines(std::vector<CString>& case_lines, Pre74_CaseLevel* pCaseLevel) const;

    Pre74_CaseLevel* FindParentLevel(Pre74_CaseLevel* pCaseLevel,Pre74_CaseLevel* pParentLevel);

    int GetNumberNodes(Pre74_CaseLevel* pCaseLevel) const;
    bool GetCaseLevelNodeNumber(Pre74_CaseLevel* pSearchCaseLevel,Pre74_CaseLevel* pCurrentCaseLevel,int& iNumber) const;
    Pre74_CaseLevel* GetCaseLevelAtNodeNumber(Pre74_CaseLevel* pCurrentCaseLevel,int& iNode);

    int CalculateRecordsForWriting(Pre74_CaseLevel* pCaseLevel) const;

private:
    const CDataDict* m_pDict;
    Pre74_CaseLevel* m_pRootLevel;

    // variables for constructing cases; the start variables are 0-based
    int m_iRecTypeLen;
    int m_iRecTypeStart;

    struct RecordTypes
    {
        TCHAR* Type;
        int Level;
        int Record;
    };

    RecordTypes* m_aRecTypes;
    int m_iNumRecTypes;
    int m_iLastRecTypePtr;

    int m_iMinLineLen;

    struct LevelKeys
    {
        struct LevelKeysItem
        {
            int Start;
            int Length;
        };

        int NumKeys;
        int KeyLen;
        LevelKeysItem* Keys;
    };

    int m_iNumLevels;
    LevelKeys* m_aLevelKeys;

    int m_iLineLen;
    const TCHAR* m_pszLine;

    int m_iLevel;
    int m_iRecord;
};
