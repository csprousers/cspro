#pragma once

//***************************************************************************
//  File name: DictionaryValidator.h
//
//  Description:
//       Header for Data Dictionary rule manager classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              02 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************
//
//  class DictionaryValidator
//
//  Description:
//      A data dictionary rule manager
//
//  Construction/Destruction
//      DictionaryValidator          Constructor
//
//  Extraction
//      GetInvalidEdit      Get ordinal position of first error
//      GetErrorReport      Get string reporting errors found
//
//  Assignment
//      SetCurrDict         Set the current dictionary
//      SetInvalidEdit      Set ordinal position of first error
//
//  Validation
//      IsValidSave         Are questionnaire ids set correctly for multiple records
//      IsValid             Is dictionary object (DataDict, Record, Item, Value Set, Value) valid
//
//  Item processing
//    void AdjustStartPositions();
//    bool IsSorted(int iRecordNum);
//    void Sort(int iRecordNum);
//
//  Subitems
//    int GetNumSubitems(int iRec, int iItem);
//
//  Generate default values
//    int  GetDefaultItemStart(int iRecordNum, int iItemNum);
//    CString GetDefaultName(const CString&);
//    CString GetDefaultRecTypeVal();
//    CalculateRecLen       Calculate length of a given record
//
//***************************************************************************

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>

/////////////////////////////////////////////////////////////////////////////
//
//                               DictionaryValidator
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZDICTO DictionaryValidator
{
// Methods
public:
// Construction
    DictionaryValidator();

// Extraction
    int GetInvalidEdit() const            { return m_iInvalidEdit;  }
    const CString& GetErrorReport() const { return m_csErrorReport; }

// Assignment
    void SetCurrDict(CDataDict* pCurrDict)
    {
        ASSERT(pCurrDict);
        m_pDict = pCurrDict;
    }
    void SetInvalidEdit(int iInvalidEdit) { m_iInvalidEdit = iInvalidEdit; }

// Validation
    bool IsValidSave(const CDataDict& dictionary);


    // Data Dicitonary rules
public:
    bool IsValid(CDataDict*,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false,
                 bool bQuick=false);

private:
    bool CheckLabel(CDataDict* pDict);
    bool CheckName(CDataDict* pDict);
    bool CheckRecTypeStart(CDataDict& dictionary);
    bool CheckRecTypeLen(CDataDict* pDict);
    bool CheckRecTypeStartLen(CDataDict* pDict);


    // Level rules
public:
    bool IsValid(DictLevel& dict_level,
                 int iLevelNum,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false,
                 bool bQuick=false);

private:
    bool CheckLabel(DictLevel& dict_level);
    bool CheckName(DictLevel& dict_level);


    // Record rules
public:
    bool IsValid(CDictRecord* pRec,
                 int iLevelNum,
                 int iRecordNum,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false,
                 bool bQuick=false);

private:
    bool CheckLabel(CDictRecord* pRec);
    bool CheckName(CDictRecord* pRec);
    bool CheckRecTypeVal(CDictRecord* pRec);
    bool CheckRequired(CDictRecord* pRec);
    bool CheckMaxRecs(CDictRecord* pRec);


    // Item rules
public:
    bool IsValidPre(CDictItem* pItem,
                    int iLevelNum,
                    int iRecordNum,
                    int iItemNum,
                    bool bSilent=false);

    bool IsValid(CDictItem* pItem,
                 int iLevelNum,
                 int iRecordNum,
                 int iItemNum,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false,
                 bool bQuick=false);

private:
    bool CheckLabel(CDictItem* pItem);
    bool CheckName(CDictItem* pItem);
    bool CheckStart(CDictItem* pItem);
    bool CheckLen(CDictItem* pItem);
    bool CheckDataType(CDictItem& dict_item);
    bool CheckItemType(CDictItem* pItem);
    bool CheckOccurs(CDictItem* pItem);
    bool CheckDecimal(CDictItem* pItem);
    bool CheckDecimalChar(CDictItem* pItem);
    bool CheckZeroFill(CDictItem* pItem);
    bool CheckCaptureInfo(CDictItem* pItem);
    bool CheckSubitem(CDictItem* pItem);
    bool CheckStartLen(CDictItem* pItem);


    // Value set rules
public:
    bool IsValid(DictValueSet& dict_value_set,
                 int iLevelNum,
                 int iRecordNum,
                 int iItemNum,
                 int iVSetNum,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false);

private:
    bool CheckLabel(DictValueSet& dict_value_set);
    bool CheckName(DictValueSet& dict_value_set);
    bool CheckValueType(DictValueSet& dict_value_set);


    // Value rules
public:
    bool IsValid(DictValue& dict_value,
                 int iLevelNum,
                 int iRecordNum,
                 int iItemNum,
                 int iVSetNum,
                 int iValueNum,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false);

private:
    bool CheckLabel(DictValue& dict_value);
    bool CheckValueType(DictValue& dict_value);


    // Value pair rules
public:
    bool IsValid(DictValuePair& dict_value_pair,
                 int iLevelNum,
                 int iRecordNum,
                 int iItemNum,
                 int iVSetNum,
                 int iValue,
                 bool bSilent=false,
                 bool bAutoFixAndRecurse=false);

private:
    bool CheckDataType(DictValuePair& dict_value_pair, bool& bIsFromField);
    bool CheckLen(DictValuePair& dict_value_pair, bool& bIsFromField);
    bool CheckFromTo(DictValuePair& dict_value_pair);


    // Relation rules
private:
    bool IsValid(const DictRelation& dict_relation, bool bSilent, bool bAutoFixAndRecurse);
    bool CheckName(const DictRelation& dict_relation);
    bool CheckPrimaryName(const DictRelation& dict_relation);
    bool CheckPrimaryLink(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part);
    bool CheckSecondaryName(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part);
    bool CheckSecondaryLink(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part);
    bool CheckLinks(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part);

    // General rules
public:
    bool CheckAliases(DictNamedBase& dict_element, bool throw_error = false, const std::set<CString>* new_aliases_to_check = nullptr);
    bool CheckNote(DictBase& dict_base);

// Item processing
public:
    int FindNewPos(int iStart, int iItem, CDictRecord* pRec);

    static void AdjustStartPositions(CDataDict& dictionary);
    void AdjustStartPositions() { AdjustStartPositions(*m_pDict); }

    bool IsSorted(int iLevelNum, int iRecordNum);
    void Sort(int iLevelNum, int iRecordNum);

// Subitem promotion
public:
    int GetNumSubitems(int iLevel, int iRec, int iItem);

// Generate default values
public:
    int  GetDefaultItemStart(int iLevelNum, int iRecordNum, int iItemNum);
    CString GetDefaultName(const CString& label);

    static bool MakeRecordTypeUnique(const CDataDict& dictionary, CString& record_type, const std::set<CString>& additional_record_types);
    CString GetDefaultRecTypeVal() const;

    static unsigned GetRecordLength(const CDataDict& dictionary, const CDictRecord* dict_record);
    unsigned GetRecordLength(const CDictRecord* dict_record) const { return GetRecordLength(*m_pDict, dict_record); }
    unsigned GetRecordLength(int iLevel, int iRec) const;
    UINT GetMaxRecLen() const;

    static void ResetRecLens(CDataDict& dictionary);
    void ResetRecLens() { ResetRecLens(*m_pDict); }

    bool AdjustValues(DictValueSet& dict_value_set);

// Data Members
private:
    CDataDict* m_pDict;

    int m_iLevelNum;
    int m_iRecordNum;
    int m_iItemNum;
    int m_iVSetNum;
    int m_iValueNum;

    bool m_bSilent;             // TRUE no messages are given.
    bool m_bAutoFixAndRecurse;  // TRUE if we auto-correct & search rec->item->vset->etc.
    int m_iInvalidEdit;         // The offending edit control
    CString m_csErrorReport;
};
