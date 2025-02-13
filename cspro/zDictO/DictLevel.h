#pragma once

#include <zDictO/DictNamedBase.h>
#include <zDictO/DictRecord.h>

class ProgressDlg;

#define COMMON -2


class CLASS_DECL_ZDICTO DictLevel : public DictNamedBase 
{
public:
    DictLevel() noexcept;
    DictLevel(const DictLevel& rhs) noexcept;
    DictLevel(DictLevel&& rhs) noexcept;
    ~DictLevel();

    DictLevel& operator=(const DictLevel& rhs) noexcept;

    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<DictLevel>(*this); }

    DictElementType GetElementType() const override { return DictElementType::Level; }

    // level numbers are zero-based
    size_t GetLevelNumber() const            { return m_levelNumber; }
    void SetLevelNumber(size_t level_number) { m_levelNumber = level_number; }


    const CDictRecord*  GetIdItemsRec  () const { return &m_idItemsDictRecord; }
    CDictRecord*        GetIdItemsRec  ()       { return &m_idItemsDictRecord; }

    int                 GetNumRecords  () const { return m_iNumRecords; }

    const CDictRecord*  GetRecord      (int i) const { return ( i == COMMON ) ? GetIdItemsRec() : m_aDictRecord[i]; }
    CDictRecord*        GetRecord      (int i)       { return ( i == COMMON ) ? GetIdItemsRec() : m_aDictRecord[i]; }

    // Assignment
    void AddRecord       (const CDictRecord* pIdItemsRec);
    void InsertRecordAt  (int iIndex, const CDictRecord* pRecord);
    void RemoveAllRecords();
    void RemoveRecordAt  (int iIndex);


    // serialization
    // ------------------------------
    static DictLevel CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    size_t m_levelNumber;
    CDictRecord m_idItemsDictRecord;

    int                       m_iNumRecords;  // Number of records
    std::vector<CDictRecord*> m_aDictRecord;  // Array of records
};
