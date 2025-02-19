#include "stdafx.h"
#include "CaseItemIndex.h"
#include "CaseItem.h"


const Case& CaseItemIndex::GetCase() const
{
    return m_caseRecord.GetCaseLevel().GetCase();
}


size_t CaseItemIndex::GetItemSubitemOccurrence(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().HasSubitemOccurrences() ? GetSubitemOccurrence() :
                                                                    GetItemOccurrence();
}


void CaseItemIndex::SetItemSubitemOccurrence(const CaseItem& case_item, const size_t item_subitem_occurrence)
{
    SetItemSubitemOccurrenceWorker(case_item.GetItemIndexHelper().HasSubitemOccurrences(), item_subitem_occurrence);
}


void CaseItemIndex::IncrementItemSubitemOccurrence(const CaseItem& case_item)
{
    if( case_item.GetItemIndexHelper().HasSubitemOccurrences() )
    {
        IncrementSubitemOccurrence();
    }

    else
    {
        IncrementItemOccurrence();
    }
}


void CaseItemIndex::DecrementItemSubitemOccurrence(const CaseItem& case_item)
{
    if( case_item.GetItemIndexHelper().HasSubitemOccurrences() )
    {
        SetSubitemOccurrence(GetSubitemOccurrence() - 1);
    }

    else
    {
        SetItemOccurrence(GetItemOccurrence() - 1);
    }
}


bool CaseItemIndex::IsValid(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().IsValid(*this);
}


std::string CaseItemIndex::GetFullOccurrencesText(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().GetFullOccurrencesText(*this);
}


std::string CaseItemIndex::GetMinimalOccurrencesText(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().GetMinimalOccurrencesText(*this);
}


bool CaseItemIndex::SetOccurrencesFromText(const CaseItem& case_item, const std::string_view occurrences_text_sv)
{
    return case_item.GetItemIndexHelper().SetOccurrencesFromText(*this, occurrences_text_sv);
}


std::string CaseItemIndex::GetSerializableText(const CaseItem& case_item) const
{
    // the string is a representation of the item name, the occurrences, and the level key
    return SO::Concatenate(case_item.GetDictItem().GetName(),
                           GetFullOccurrencesText(case_item),
                           UTF8_TODO::GetUtf8(m_caseRecord.GetCaseLevel().GetLevelKey()));
}


std::tuple<const CaseItem*, std::unique_ptr<CaseItemIndex>> CaseItemIndex::FromSerializableText(const Case& data_case, const std::string_view serializable_text_sv)
{
    const auto [left_parenthesis_pos, right_parenthesis_pos] = SO::FindCharacters(serializable_text_sv, '(', ')');

    if( right_parenthesis_pos != std::string_view::npos )
    {
        // find the case item, which comes before the left parenthesis
        const std::string_view item_name_sv = serializable_text_sv.substr(0, left_parenthesis_pos);
        const CaseItem* const case_item = data_case.GetCaseMetadata().FindCaseItem(item_name_sv);

        if( case_item != nullptr )
        {
            const CaseRecordMetadata* const case_record_metadata = data_case.GetCaseMetadata().FindCaseRecordMetadata(case_item->GetDictItem().GetRecord()->GetName());
            ASSERT(case_record_metadata != nullptr);

            // find the matching case level
            const std::string_view level_key_sv = serializable_text_sv.substr(right_parenthesis_pos + 1);

            for( const CaseLevel* const case_level : data_case.GetAllCaseLevels() )
            {
                if( UTF8_TODO::GetUtf8(case_level->GetLevelKey()) == level_key_sv )
                {
                    const CaseRecord& case_record = case_level->GetCaseRecord(case_record_metadata->GetRecordIndex());

                    // create the index and return it if it is valid
                    std::unique_ptr<CaseItemIndex> index(new CaseItemIndex(case_record, 0));
                    const std::string_view occurrences_text_sv = serializable_text_sv.substr(left_parenthesis_pos, right_parenthesis_pos - left_parenthesis_pos + 1);

                    if( index->SetOccurrencesFromText(*case_item, occurrences_text_sv) )
                        return std::make_tuple(case_item, std::move(index));
                }
            }
        }
    }

    return std::tuple<const CaseItem*, std::unique_ptr<CaseItemIndex>>(nullptr, nullptr);
}
