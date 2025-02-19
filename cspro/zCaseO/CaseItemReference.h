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
    CaseItemReference(const CaseItem& case_item, std::string level_key, size_t record_occurrence = 0, size_t item_occurrence = 0, size_t subitem_occurrence = 0);
    CaseItemReference(const CaseItem& case_item, std::string level_key, const size_t occurrences[NumberDimensions]);

    const CaseItem& GetCaseItem() const               { return m_caseItem; }
    const ItemIndexHelper& GetItemIndexHelper() const { return m_caseItem.GetItemIndexHelper(); }

    bool HasOccurrences() const override;

    std::string GetMinimalOccurrencesText() const override;

    const size_t* GetZeroBasedOccurrences() const override { return GetOccurrences(); }

    std::vector<size_t> GetOneBasedOccurrences() const override;

    bool OnCaseLevel(const std::string& level_key) const { return ( GetLevelKey() == level_key ); }
    bool OnCaseLevel(const CaseLevel& case_level) const;
    bool OnCaseLevel(const CaseRecord& case_record) const;

    template<typename T>
    bool Equals(const std::string& name, const T& case_level_t, const ItemIndex& index) const;

    bool Equals(const CaseItem& case_item, const CaseItemIndex& index) const;

protected:
    bool OccurrencesMatch(const NamedReference& rhs) const override;

private:
    const CaseItem& m_caseItem;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
bool CaseItemReference::Equals(const std::string& name, const T& case_level_t, const ItemIndex& index) const
{
    return ( ( GetName() == name ) &&
             ( OnCaseLevel(case_level_t) ) &&
             ( static_cast<const ItemIndex&>(*this) == index ) );
}


inline bool CaseItemReference::Equals(const CaseItem& case_item, const CaseItemIndex& index) const
{
    return Equals(case_item.GetDictItem().GetName(), index.GetCaseRecord(), index);
}
