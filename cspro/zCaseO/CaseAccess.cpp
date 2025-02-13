#include "stdafx.h"
#include "CaseAccess.h"
#include <zToolsO/Serializer.h>


CaseAccess::CaseAccess(const CDataDict& dictionary)
    :   m_dictionary(dictionary),
        m_usesNotes(false),
        m_usesStatuses(false),
        m_usesCaseLabels(false),
        m_usesGetBuffer(false),
        m_usedDictItems(std::make_unique<std::set<CString>>()),
        m_initialized(false),
        m_caseMetadata(nullptr)
{
    // if not using read optimizations, mark the entire dictionary as requiring access
    if( !m_dictionary.GetReadOptimization() )
    {
        SetRequiresFullAccess();
        return;
    }

    // otherwise, mark the ID items from a dictionary as always used
    for( const DictLevel& dict_level : dictionary.GetLevels() )
    {
        const CDictRecord& id_dict_record = *dict_level.GetIdItemsRec();

        for( int item_counter = 0; item_counter < id_dict_record.GetNumItems(); ++item_counter )
        {
            const CDictItem& dict_item = *(id_dict_record.GetItem(item_counter));
            SetUseDictionaryItem(dict_item);
        }
    }
}


CaseAccess::~CaseAccess()
{
}


std::unique_ptr<CaseAccess> CaseAccess::CreateAndInitializeFullCaseAccess(const CDataDict& dictionary)
{
    auto case_access = std::make_unique<CaseAccess>(dictionary);

    case_access->SetRequiresFullAccess();
    case_access->Initialize();

    return case_access;
}


void CaseAccess::SetRequiresFullAccess()
{
    SetUsesAllCaseAttributes();
    SetUseAllDictionaryItems();
}


void CaseAccess::SetUsesAllCaseAttributes()
{
    SetUsesNotes();
    SetUsesStatuses();
    SetUsesCaseLabels();

    // SetUsesGetBuffer is not called because that is a
    // very specific attribute and not a general case attribute
}


bool CaseAccess::GetUseDictionaryItem(const CDictItem& dict_item) const
{
    ASSERT(!IsInitialized());

    return ( ( m_usedDictItems == nullptr ) ||
             ( m_usedDictItems->find(dict_item.GetName()) != m_usedDictItems->end() ) );
}


void CaseAccess::SetUseDictionaryItem(const CDictItem& dict_item)
{
    ASSERT(!IsInitialized());

    if( m_usedDictItems != nullptr )
    {
        // parent items will be added for subitems (which will end up adding the subitems)
        if( dict_item.GetItemType() == ItemType::Subitem )
        {
            SetUseDictionaryItem(*(dict_item.GetParentItem()));
        }

        else
        {
            m_usedDictItems->insert(dict_item.GetName());

            // subitems will be added for parent items
            const CDictRecord& dict_record = *(dict_item.GetRecord());

            for( int item_index = dict_item.GetSonNumber() + 1; item_index < dict_record.GetNumItems(); ++item_index )
            {
                const CDictItem& potential_child_dict_item = *(dict_record.GetItem(item_index));

                if( potential_child_dict_item.GetParentItem() != &dict_item )
                    break;

                m_usedDictItems->insert(potential_child_dict_item.GetName());
            }
        }
    }
}


void CaseAccess::SetUseAllDictionaryItems()
{
    ASSERT(!IsInitialized());

    m_usedDictItems.reset();
}


void CaseAccess::Initialize()
{
    if( !IsInitialized() )
    {
        m_caseMetadata = std::make_unique<CaseMetadata>(m_dictionary, *this);

        // setup the case item lookup
        auto add_case_items_from_record = [&](const CaseRecordMetadata* case_record_metadata)
        {
            for( const CaseItem* case_item : case_record_metadata->GetCaseItems() )
                m_caseItemLookup.emplace(case_item->GetDictionaryItem().GetName(), case_item);
        };

        for( const CaseLevelMetadata* case_level_metadata : m_caseMetadata->GetCaseLevelsMetadata() )
        {
            add_case_items_from_record(case_level_metadata->GetIdCaseRecordMetadata());

            for( const CaseRecordMetadata* case_record_metadata : case_level_metadata->GetCaseRecordsMetadata() )
                add_case_items_from_record(case_record_metadata);
        }

        m_initialized = true;
    }
}


const CaseMetadata& CaseAccess::GetCaseMetadata() const
{
    ASSERT(IsInitialized());

    return *m_caseMetadata;
}


const CaseItem* CaseAccess::LookupCaseItem(const CString& item_name) const
{
    ASSERT(IsInitialized());

    auto case_item_search = m_caseItemLookup.find(item_name);
    return ( case_item_search != m_caseItemLookup.end() ) ? case_item_search->second : nullptr;
}


const CaseItem* CaseAccess::LookupCaseItem(const CDictItem& dict_item) const
{
    const CaseItem* case_item = LookupCaseItem(dict_item.GetName());

    // if here, make sure that you call SetUseDictionaryItem with the dictionary item
    ASSERT(case_item != nullptr);

    return case_item;
}


std::optional<std::wstring> CaseAccess::GetUnsupportedContentTypesString(const std::set<CaseItem::Type>& supported_case_item_types) const
{
    ASSERT(IsInitialized());

    std::set<ContentType> unsupported_content_types;

    for( const auto& [item_name, case_item] : m_caseItemLookup )
    {
        if( supported_case_item_types.find(case_item->GetType()) == supported_case_item_types.cend() )
            unsupported_content_types.emplace(case_item->GetDictionaryItem().GetContentType());
    }

    if( unsupported_content_types.empty() )
        return std::nullopt;

    std::vector<std::wstring> content_type_names;

    for( const ContentType content_type : unsupported_content_types )
        content_type_names.emplace_back(ToString(content_type));

    return SO::CreateSingleString(content_type_names);
}


void CaseAccess::IssueWarningIfUsingUnsupportedCaseItems(const std::set<CaseItem::Type>& supported_case_item_types) const
{
    ASSERT(IsInitialized());

    if( m_caseConstructionReporter == nullptr )
        return;

    const std::optional<std::wstring> unsupported_content_types_string = GetUnsupportedContentTypesString(supported_case_item_types);

    if( unsupported_content_types_string.has_value() )
        m_caseConstructionReporter->UnsupportedContentType(m_dictionary.GetName(), *unsupported_content_types_string);
}


std::unique_ptr<Case> CaseAccess::CreateCase(const bool set_case_construction_reporter/* = false*/) const
{
    ASSERT(IsInitialized());

    auto data_case = std::make_unique<Case>(GetCaseMetadata());

    if( set_case_construction_reporter )
        data_case->SetCaseConstructionReporter(m_caseConstructionReporter);

    return data_case;
}


void CaseAccess::serialize(Serializer& ar)
{
    ASSERT(!IsInitialized() && ( m_usedDictItems != nullptr || ar.IsSaving() ));

    ar & m_usesNotes
       & m_usesStatuses
       & m_usesCaseLabels
       & m_usesGetBuffer;

    bool use_all_dict_items = ( m_usedDictItems == nullptr );
    ar & use_all_dict_items;

    if( use_all_dict_items )
    {
        if( ar.IsLoading() )
            SetUseAllDictionaryItems();
    }

    else
    {
        ar & *m_usedDictItems;
    }
}
