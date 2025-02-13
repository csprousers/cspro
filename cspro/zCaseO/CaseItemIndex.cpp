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


void CaseItemIndex::SetItemSubitemOccurrence(const CaseItem& case_item, size_t item_subitem_occurrence)
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


CString CaseItemIndex::GetFullOccurrencesText(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().GetFullOccurrencesText(*this);
}


CString CaseItemIndex::GetMinimalOccurrencesText(const CaseItem& case_item) const
{
    return case_item.GetItemIndexHelper().GetMinimalOccurrencesText(*this);
}


bool CaseItemIndex::SetOccurrencesFromText(const CaseItem& case_item, const TCHAR* occurrences_text)
{
    return case_item.GetItemIndexHelper().SetOccurrencesFromText(*this, occurrences_text);
}


std::wstring CaseItemIndex::GetSerializableText(const CaseItem& case_item) const
{
    // the string is a representation of the item name, the occurrences, and the level key
    return FormatTextCS2WS(_T("%s%s%s"), case_item.GetDictionaryItem().GetName().GetString(),
                                         GetFullOccurrencesText(case_item).GetString(),
                                         m_caseRecord.GetCaseLevel().GetLevelKey().GetString());
}


std::tuple<const CaseItem*, std::unique_ptr<CaseItemIndex>> CaseItemIndex::FromSerializableText(const Case& data_case, wstring_view serializable_text_sv)
{
    const size_t left_parenthesis = serializable_text_sv.find('(');

    if( left_parenthesis != wstring_view::npos )
    {
        const size_t right_parenthesis = serializable_text_sv.find(')', left_parenthesis + 1);

        if( right_parenthesis != wstring_view::npos )
        {
            // find the case item, which comes before the left parenthesis
            const wstring_view item_name_sv = serializable_text_sv.substr(0, left_parenthesis);
            const CaseItem* case_item = data_case.GetCaseMetadata().FindCaseItem(item_name_sv);

            if( case_item != nullptr )
            {
                const CaseRecordMetadata* case_record_metadata = data_case.GetCaseMetadata().FindCaseRecordMetadata(case_item->GetDictionaryItem().GetRecord()->GetName());
                ASSERT(case_record_metadata != nullptr);
                
                // find the matching case level
                const wstring_view level_key_sv = serializable_text_sv.substr(right_parenthesis + 1);

                for( const CaseLevel* case_level : data_case.GetAllCaseLevels() )
                {
                    if( SO::Equals(level_key_sv, case_level->GetLevelKey()) )
                    {
                        const CaseRecord& case_record = case_level->GetCaseRecord(case_record_metadata->GetRecordIndex());

                        // create the index and return it if it is valid
                        auto index = std::unique_ptr<CaseItemIndex>(new CaseItemIndex(case_record, 0));
                        wstring_view occurrences_text_sv = serializable_text_sv.substr(left_parenthesis, right_parenthesis - left_parenthesis + 1);

                        if( index->SetOccurrencesFromText(*case_item, CString(occurrences_text_sv)) )
                            return std::make_tuple(case_item, std::move(index));
                    }
                }
            }
        }
    }

    return std::tuple<const CaseItem*, std::unique_ptr<CaseItemIndex>>(nullptr, nullptr);
}
