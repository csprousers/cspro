#pragma once

#include <zDictO/CaptureInfo.h>
#include <zDictO/DictNamedBase.h>
#include <zDictO/DictValueSet.h>
#include <zUtilO/DataTypes.h>
#include <zAppO/OccurrenceLabels.h>

class CDictRecord;
class DictLevel;


enum class ItemType { Item, Subitem };

const int MAX_OCCURS = 9999;


class CLASS_DECL_ZDICTO CDictItem : public DictNamedBase 
{
public:
    // Construction/destruction
    CDictItem();
    CDictItem(const CDictItem& rhs);

    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<CDictItem>(*this); }

    DictElementType GetElementType() const override { return DictElementType::Item; }

    // Extraction
    CString GetQualifiedName() const;
    UINT                GetStart   () const { return m_uStart; }


    UINT                GetLen     () const { return m_uLen; }
    // GetLen does not consider the dot for number with decimal
    // places and explicit dot. GetCompleteLen() fixes this
    UINT                GetCompleteLen() const; // rcl, Apr 2005
    UINT                GetIntegerLen() const;
    ContentType         GetContentType() const { return m_contentType; }
    bool                AddToTreeFor80() const { return !IsBinary(m_contentType); } // BINARY_TYPES_TO_ENGINE_TODO for 8.0, binary items aren't generally added to the tree
    DataType            GetDataType() const;
    ItemType            GetItemType() const    { return m_itemType; }
    bool                IsSubitem() const      { return ( m_itemType == ItemType::Subitem ); }
    UINT                GetOccurs  () const    { return m_uOccurs; }
    UINT                GetItemSubitemOccurs() const;
    UINT                GetDecimal () const    { return m_uDecimal; }
    bool                GetDecChar () const    { return m_bDecChar; }
    bool                GetZeroFill() const    { return m_bZeroFill; }

    CaptureInfo&        GetCaptureInfo()       { return m_captureInfo; }
    const CaptureInfo&  GetCaptureInfo() const { return m_captureInfo; }

    int                 GetSymbol       () const { return m_iSymbol; }
    const CDictItem*    GetParentItem   () const { return m_pParentItem; }          // Parent item of a sub-item. NULL if I'm an item.// RHF Nov 23, 2000
    CDictItem*          GetParentItem   ()       { return m_pParentItem; }          // Parent item of a sub-item. NULL if I'm an item.// RHF Nov 23, 2000
    const DictLevel*    GetLevel        () const { return m_pLevel; }               // Use unknown
    DictLevel*          GetLevel        ()       { return m_pLevel; }               // Use unknown
    const CDictRecord*  GetRecord       () const { return m_pRecord; }              // Use unknown
    CDictRecord*        GetRecord       ()       { return m_pRecord; }              // Use unknown
    int                 GetSonNumber    () const { return m_iSonNumber; }           // Use unknown
    size_t              GetItemNumber   () const { return (size_t)m_iSonNumber; }

    // Assignment
    void SetContentType (ContentType content_type)        { m_contentType = content_type; }
    void SetItemType    (ItemType itemType)               { m_itemType = itemType; }
    void SetStart       (UINT uStart)                     { m_uStart = uStart; }
    void SetLen         (UINT uLen)                       { m_uLen = uLen; }
    void SetOccurs      (UINT uOccurs)                    { m_uOccurs = uOccurs; m_occurrenceLabels.DeleteOccurrencesBeyond(m_uOccurs); }
    void SetDecimal     (UINT uDecimal)                   { m_uDecimal = uDecimal; }
    void SetDecChar     (bool bDecChar)                   { m_bDecChar = bDecChar; }
    void SetZeroFill    (bool bZeroFill)                  { m_bZeroFill = bZeroFill; }
    void SetCaptureInfo (const CaptureInfo& capture_info) { m_captureInfo = capture_info; }

    void SetSymbol      (int iSymbol) { m_iSymbol = iSymbol; }                          // Use unknown
    void SetParentItem  (CDictItem* pParentItem) { m_pParentItem = pParentItem; }       // Parent item of a sub-item. NULL if I'm an item.// RHF Nov 23, 2000
    void SetLevel       (DictLevel* pLevel) { m_pLevel = pLevel; }                      // Used by UpdataPointers only
    void SetRecord      (CDictRecord* pRecord) { m_pRecord = pRecord; }                 // Used by UpdataPointers only
    void SetSonNumber   (int iSonNumber) {m_iSonNumber = iSonNumber; }                  // Used by UpdataPointers only

    bool HasSubitems() const;


    // Operators
    void operator=(const CDictItem& item);

private:
    UINT                            m_uStart;       // Start position (1-based)
    UINT                            m_uLen;         // Length
    ContentType                     m_contentType;  // Content type
    ItemType                        m_itemType;     // Item type
    UINT                            m_uOccurs;      // Number of occurrences
    UINT                            m_uDecimal;     // Number of decimal places
    bool                            m_bDecChar;     // TRUE if decimal character is present
    bool                            m_bZeroFill;    // TRUE if field must be zero filled

    CaptureInfo                     m_captureInfo;

    // None of the following are used by the dictionary program
    int                             m_iSymbol;      // Use unknown
    CDictItem*                      m_pParentItem;  // Parent item of a sub-item. NULL if I'm an item.// RHF Nov 23, 2000
    DictLevel*                      m_pLevel;       // Pointer to parent dictionary level
    CDictRecord*                    m_pRecord;      // Pointer to parent dictionary record
    int                             m_iSonNumber;   // Index of item in this record (0-based)

    // ...

public:
    // occurrence labels
    // ------------------------------
    const OccurrenceLabels& GetOccurrenceLabels() const { return m_occurrenceLabels; }
    OccurrenceLabels& GetOccurrenceLabels()             { return m_occurrenceLabels; }


    // value sets
    // ------------------------------
    size_t GetNumValueSets() const { return m_dictValueSets.size(); }
    bool HasValueSets() const      { return !m_dictValueSets.empty(); }

    const std::vector<DictValueSet>& GetValueSets() const { return m_dictValueSets; }
    std::vector<DictValueSet>& GetValueSets()             { return m_dictValueSets; }

    const DictValueSet& GetValueSet(size_t index) const   { ASSERT(index < m_dictValueSets.size()); return m_dictValueSets[index]; }
    DictValueSet& GetValueSet(size_t index)               { ASSERT(index < m_dictValueSets.size()); return m_dictValueSets[index]; }

    const DictValueSet* GetFirstValueSetOrNull() const    { return m_dictValueSets.empty() ? nullptr : &m_dictValueSets.front(); }

    void AddValueSet(DictValueSet dict_value_set);
    void InsertValueSet(size_t index, DictValueSet dict_value_set);
    void RemoveValueSet(size_t index);
    void RemoveAllValueSets();


    // serialization
    // ------------------------------
    static CDictItem CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    OccurrenceLabels m_occurrenceLabels;
    std::vector<DictValueSet> m_dictValueSets;
};


inline bool IsNumeric(const CDictItem& dict_item) { return IsNumeric(dict_item.GetContentType()); }
inline bool IsString(const CDictItem& dict_item)  { return IsString(dict_item.GetContentType());  }
inline bool IsBinary(const CDictItem& dict_item)  { return IsBinary(dict_item.GetContentType());  }
