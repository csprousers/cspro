#include "stdafx.h"
#include "CaseItemReference.h"


CaseItemReference::CaseItemReference(const CaseItem& case_item, const CString& level_key,
    size_t record_occurrence/* = 0*/, size_t item_occurrence/* = 0*/, size_t subitem_occurrence/* = 0*/)
    :   NamedReference(case_item.GetDictionaryItem().GetName(), level_key),
        ItemIndex(record_occurrence, item_occurrence, subitem_occurrence),
        m_caseItem(case_item)
{
}

CaseItemReference::CaseItemReference(const CaseItem& case_item, const CString& level_key, const size_t occurrences[NumberDimensions])
    :   NamedReference(case_item.GetDictionaryItem().GetName(), level_key),
        ItemIndex(occurrences),
        m_caseItem(case_item)
{
}

bool CaseItemReference::HasOccurrences() const
{
    return GetItemIndexHelper().HasOccurrences();
}

CString CaseItemReference::GetMinimalOccurrencesText() const
{
    return GetItemIndexHelper().GetMinimalOccurrencesText(*this);
}

std::vector<size_t> CaseItemReference::GetOneBasedOccurrences() const
{
    return GetItemIndexHelper().GetOneBasedOccurrences(*this);
}

bool CaseItemReference::OnCaseLevel(const CString& level_key) const
{
    return ( m_levelKey.Compare(level_key) == 0 );
}

bool CaseItemReference::OnCaseLevel(const CaseLevel& case_level) const
{
    return OnCaseLevel(case_level.GetLevelKey());
}

bool CaseItemReference::OnCaseLevel(const CaseRecord& case_record) const
{
    return OnCaseLevel(case_record.GetCaseLevel());
}

bool CaseItemReference::OccurrencesMatch(const NamedReference& rhs) const
{
    const size_t* rhs_occurrences = rhs.GetZeroBasedOccurrences();

    if( rhs_occurrences == nullptr )
        return false;

    return ( memcmp(m_occurrences, rhs_occurrences, sizeof(m_occurrences)) == 0 );
}
