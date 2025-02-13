#pragma once

#include <zCaseO/NamedReference.h>
#include <zCaseO/CaseItem.h>
#include <zDictO/ItemIndex.h>

class CaseLevel;
class CaseRecord;


// Reference to a CaseItem
class ZCASEO_API CaseItemReference : public NamedReference, public ItemIndex
{
public:
    CaseItemReference(const CaseItem& case_item, const CString& level_key, size_t record_occurrence = 0, size_t item_occurrence = 0, size_t subitem_occurrence = 0);
    CaseItemReference(const CaseItem& case_item, const CString& level_key, const size_t occurrences[NumberDimensions]);

    const CaseItem& GetCaseItem() const               { return m_caseItem; }
    const ItemIndexHelper& GetItemIndexHelper() const { return m_caseItem.GetItemIndexHelper(); }

    bool HasOccurrences() const override;

    CString GetMinimalOccurrencesText() const override;

    const size_t* GetZeroBasedOccurrences() const override { return GetOccurrences(); }

    std::vector<size_t> GetOneBasedOccurrences() const override;

    bool OnCaseLevel(const CString& level_key) const;
    bool OnCaseLevel(const CaseLevel& case_level) const;
    bool OnCaseLevel(const CaseRecord& case_record) const;

    template<typename T>
    bool Equals(const CString& name, const T& case_level_t, const ItemIndex& index) const
    {
        return ( ( m_name.Compare(name) == 0 ) &&
                 ( OnCaseLevel(case_level_t) ) &&
                 ( ((const ItemIndex&)*this) == index ) );
    }

    bool Equals(const CaseItem& case_item, const CaseItemIndex& index) const
    {
        return Equals(case_item.GetDictionaryItem().GetName(), index.GetCaseRecord(), index);
    }

protected:
    bool OccurrencesMatch(const NamedReference& rhs) const override;

private:
    const CaseItem& m_caseItem;
};
