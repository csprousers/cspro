#pragma once

#include <zDictO/DictNamedBase.h>
#include <zDictO/DictItem.h>
#include <zAppO/OccurrenceLabels.h>

class CDataDict;
class DictLevel;
class ProgressDlg;


constexpr int MAX_MAX_RECS = 9999;  /* the Max recs field in CDictRecord  */


class CLASS_DECL_ZDICTO CDictRecord : public DictNamedBase 
{
public:
    // Construction/destruction
    CDictRecord(bool id_record = false);
    CDictRecord(const CDictRecord& r);       // copy constructor
    ~CDictRecord();

    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<CDictRecord>(*this); }

    DictElementType GetElementType() const override { return DictElementType::Record; }

    // Extraction methods
    const CString&      GetRecTypeVal  () const { return m_csRecTypeVal;  }
    bool                GetRequired    () const { return m_bRequired; }
    UINT                GetMaxRecs     () const { return m_uMaxRecs; }
    int                 GetRecLen      () const { return m_iRecLen; }
    int                 GetNumItems    () const { return m_iNumItems;  }

    CDictItem*          GetItem (int i)       { return m_aDictItem[i];  }
    const CDictItem*    GetItem (int i) const { return m_aDictItem[i]; }

    int                 GetSymbol      () const { return m_iSymbol; }
    const CDataDict*    GetDataDict    () const { return m_pDataDict; }            // Use unknown
    DictLevel*          GetLevel       () const { return m_pLevel; }               // Use unknown
    int                 GetSonNumber   () const { return m_iSonNumber; }           // Use unknown
    size_t              GetRecordNumber() const { return ( m_iSonNumber < 0 ) ? SIZE_MAX : (size_t)m_iSonNumber; }

    const OccurrenceLabels& GetOccurrenceLabels() const { return m_occurrenceLabels; }
    OccurrenceLabels& GetOccurrenceLabels()             { return m_occurrenceLabels; }

    const CDictItem* FindItem(wstring_view item_name) const;
    CDictItem* FindItem(wstring_view item_name);

    // assignment methods
    void SetRecTypeVal  (const CString& csRecTypeVal) { m_csRecTypeVal = csRecTypeVal; }
    void SetRequired    (bool bRequired) { m_bRequired = bRequired; }
    void SetMaxRecs     (UINT uMaxRecs) { m_uMaxRecs = uMaxRecs; m_occurrenceLabels.DeleteOccurrencesBeyond(uMaxRecs); }
    void SetRecLen      (int iRecLen) { m_iRecLen = iRecLen; }

    void SetSymbol      (int iSymbol) { m_iSymbol = iSymbol; }                          // Use unknown
    void SetDataDict    (const CDataDict* pDataDict) { m_pDataDict = pDataDict; }       // Used by UpdataPointers only
    void SetLevel       (DictLevel* pLevel) { m_pLevel = pLevel; }                      // Used by UpdataPointers only
    void SetSonNumber   (int iSonNumber) {m_iSonNumber = iSonNumber; }                  // Used by UpdataPointers only

    void AddItem        (const CDictItem* pItem);
    void InsertItemAt   (int iIndex, const CDictItem* pItem);
    void RemoveAllItems();
    void RemoveItemAt   (int iIndex);

    UINT CalculateRecLen();
    bool Is2DRecord() const;

    // operators
    void operator=(const CDictRecord& rhs);

    // ...


    bool IsIdRecord() const { return m_idRecord; }


    // serialization
    // ------------------------------
    static CDictRecord CreateFromJson(const JsonNode<wchar_t>& json_node, bool id_record = false);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    // Data Members
    CString                         m_csRecTypeVal; // Record type value
    bool                            m_bRequired;    // TRUE if required
    UINT                            m_uMaxRecs;     // Maximum number of records
    int                             m_iRecLen;      // Record length
    int                             m_iNumItems;    // Number of items
    std::vector<CDictItem*>         m_aDictItem;    // Array of items

    // None of the following are used by the dictionary program
    int                             m_iSymbol;      // Use unknown
    const CDataDict*                m_pDataDict;    // Pointer to parent dictionary object
    DictLevel*                      m_pLevel;       // Pointer to parent dictionary level
    int                             m_iSonNumber;   // Index of record in this level (0-based)
    OccurrenceLabels                m_occurrenceLabels;

    // ...

    bool m_idRecord;
};
