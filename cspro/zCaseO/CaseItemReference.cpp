#include "stdafx.h"
#include "CaseItemReference.h"


CaseItemReference::CaseItemReference(const CaseItem& case_item, std::string level_key,
                                     const size_t record_occurrence/* = 0*/, const size_t item_occurrence/* = 0*/, const size_t subitem_occurrence/* = 0*/)
    :   NamedReference(case_item.GetDictItem().GetName(), std::move(level_key)),
        ItemIndex(record_occurrence, item_occurrence, subitem_occurrence),
        m_caseItem(case_item)
{
}


CaseItemReference::CaseItemReference(const CaseItem& case_item, std::string level_key, const size_t occurrences[NumberDimensions])
    :   NamedReference(case_item.GetDictItem().GetName(), std::move(level_key)),
        ItemIndex(occurrences),
        m_caseItem(case_item)
{
}


bool CaseItemReference::HasOccurrences() const
{
    return GetItemIndexHelper().HasOccurrences();
}


std::string CaseItemReference::GetMinimalOccurrencesText() const
{
    return GetItemIndexHelper().GetMinimalOccurrencesText(*this);
}


std::vector<size_t> CaseItemReference::GetOneBasedOccurrences() const
{
    return GetItemIndexHelper().GetOneBasedOccurrences(*this);
}


bool CaseItemReference::OnCaseLevel(const CaseLevel& case_level) const
{
    return OnCaseLevel(UTF8_TODO::GetUtf8(case_level.GetLevelKey()));
}


bool CaseItemReference::OnCaseLevel(const CaseRecord& case_record) const
{
    return OnCaseLevel(case_record.GetCaseLevel());
}


bool CaseItemReference::OccurrencesMatch(const NamedReference& rhs) const
{
    const size_t* const rhs_occurrences = rhs.GetZeroBasedOccurrences();

    if( rhs_occurrences == nullptr )
        return false;

    return ( memcmp(m_occurrences, rhs_occurrences, sizeof(m_occurrences)) == 0 );
}
