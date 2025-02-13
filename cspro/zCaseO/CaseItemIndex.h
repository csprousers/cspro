#pragma once

#include <zCaseO/zCaseO.h>
#include <zDictO/ItemIndex.h>

class Case;
class CaseItem;
class CaseRecord;


class ZCASEO_API CaseItemIndex : public ItemIndex
{
    friend CaseItem;
    friend CaseRecord;

private:
    CaseItemIndex(const CaseRecord& case_record, size_t record_occurrence)
        :   ItemIndex(record_occurrence),
            m_caseRecord(case_record),
            m_lastCalculatedDataBuffer(nullptr),
            m_lastCalculatedCaseItem(nullptr)
    {
    }

public:
    CaseItemIndex(const CaseItemIndex&) = default;

    // the CaseRecord will be stored as a const object so that CaseItemIndex can be const,
    // but CaseRecord::GetCaseItemIndex also has a non-const version, so we will cast
    // the CaseRecord as needed
    const CaseRecord& GetCaseRecord() const { return m_caseRecord; }
    CaseRecord& GetCaseRecord()             { return const_cast<CaseRecord&>(m_caseRecord); }

    const Case& GetCase() const;

    void OnItemIndexChange() override       { m_lastCalculatedDataBuffer = nullptr; }

    // helpers for working with item/subitem occurrences
    size_t GetItemSubitemOccurrence(const CaseItem& case_item) const;
    void SetItemSubitemOccurrence(const CaseItem& case_item, size_t item_subitem_occurrence);
    void IncrementItemSubitemOccurrence(const CaseItem& case_item);
    void DecrementItemSubitemOccurrence(const CaseItem& case_item);

    /// <summary>
    /// Returns whether the index is valid for the case item.
    /// </summary>
    bool IsValid(const CaseItem& case_item) const;

    // helpers for converting occurrences to/from text
    CString GetFullOccurrencesText(const CaseItem& case_item) const;
    CString GetMinimalOccurrencesText(const CaseItem& case_item) const;
    bool SetOccurrencesFromText(const CaseItem& case_item, const TCHAR* occurrences_text);

    // helpers for serializing case item references to/from text
    std::wstring GetSerializableText(const CaseItem& case_item) const;
    static std::tuple<const CaseItem*, std::unique_ptr<CaseItemIndex>> FromSerializableText(const Case& data_case, wstring_view serializable_text_sv);

private:
    const CaseRecord& m_caseRecord;
    mutable const std::byte* m_lastCalculatedDataBuffer;
    mutable const CaseItem* m_lastCalculatedCaseItem;
};
