#include "stdafx.h"
#include "DataFileLister.h"
#include <zDictO/DictionaryCreator.h>
#include <zCaseO/CaseItemHelpers.h>
#include <zCaseO/FixedWidthNumericCaseItem.h>
#include <zCaseO/FixedWidthStringCaseItem.h>
#include <zDataO/DataRepository.h>


namespace
{
    constexpr const TCHAR* ListingPrefix     = _T("LISTING_");

    constexpr const TCHAR* MessageTypeName   = _T("LISTING_MESSAGE_TYPE");
    constexpr const TCHAR* MessageNumberName = _T("LISTING_MESSAGE_NUMBER");
    constexpr const TCHAR* MessageTextName   = _T("LISTING_MESSAGE_TEXT");
    
    constexpr size_t MaxMessagesPerCase = 500;

    constexpr double WriteMessageTypeCode = static_cast<double>(MessageType::User) + 1;
}


// --------------------------------------------------
// DataFileLister::ClassVariables
// --------------------------------------------------

namespace Listing
{
    struct DataFileLister::ClassVariables
    {
        std::unique_ptr<CDataDict> dictionary;
        std::shared_ptr<CaseAccess> case_access;
        std::unique_ptr<DataRepository> repository;
        std::unique_ptr<Case> data_case;

        CaseRecord* id_case_record;
        CaseRecord* listing_case_record;

        struct IdLink
        {
            size_t level_number;
            const CaseItem* input_case_item;
            const CaseItem* listing_case_item;
        };

        std::vector<IdLink> id_links;

        const CaseItem* message_type_case_item;
        const CaseItem* message_number_case_item;
        const CaseItem* message_text_case_item;
    };
}



// --------------------------------------------------
// DataFileLister
// --------------------------------------------------

Listing::DataFileLister::DataFileLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, std::shared_ptr<const CaseAccess> case_access)
    :   Lister(process_summary),
        m_cv(std::make_unique<DataFileLister::ClassVariables>()),
        m_caseAccess(std::move(case_access))
{
    if( m_caseAccess == nullptr )
        throw CSProException("The data file lister can only be used when working with applications that use an input dictionary.");

    // create the dictionary and save it
    CreateAndInitializeListingDictionary(m_caseAccess->GetDataDict());

    m_cv->dictionary->Save(filename + FileExtensions::WithDot::Dictionary);

    // open the listing data file
    m_cv->repository = DataRepository::CreateAndOpen(m_cv->case_access, ConnectionString(filename),
        append ? DataRepositoryAccess::BatchOutputAppend : DataRepositoryAccess::BatchOutput,
        DataRepositoryOpenFlag::OpenOrCreate);
}


Listing::DataFileLister::~DataFileLister()
{
    // close the repository first (because it depends on m_cv->case_access)
    m_cv->repository.reset();
}


void Listing::DataFileLister::CreateAndInitializeListingDictionary(const CDataDict& source_dictionary)
{
    DictionaryCreator dictionary_creator(source_dictionary, ListingPrefix, _T("Listing"), MaxMessagesPerCase);
    dictionary_creator.AddItem(MessageTypeName, _T("Message Type"), ContentType::Numeric, 1)
                        .AddValueSet(MessageTypeName,
                            {
                                { _T("Abort"),   static_cast<double>(MessageType::Abort)   },
                                { _T("Error"),   static_cast<double>(MessageType::Error)   },
                                { _T("Warning"), static_cast<double>(MessageType::Warning) },
                                { _T("User"),    static_cast<double>(MessageType::User)    },
                                { _T("Write"),   WriteMessageTypeCode         }
                            })
                        .AddItem(MessageNumberName, _T("Message Number"), ContentType::Numeric, 15)
                        .AddItem(MessageTextName, _T("Message Text"), ContentType::Alpha, 500);

    m_cv->dictionary = dictionary_creator.GetDictionary();

    m_cv->case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*m_cv->dictionary);

    m_cv->data_case = m_cv->case_access->CreateCase();

    m_cv->id_case_record = &m_cv->data_case->GetRootCaseLevel().GetIdCaseRecord();
    m_cv->listing_case_record = &m_cv->data_case->GetRootCaseLevel().GetCaseRecord(0);

    // setup the ID links
    for( const CDictItem* id_item : source_dictionary.GetIdItems() )
    {
        m_cv->id_links.emplace_back(ClassVariables::IdLink
            {
                id_item->GetLevel()->GetLevelNumber(),
                nullptr,
                m_cv->case_access->LookupCaseItem(ListingPrefix + id_item->GetName())
            });
    }

    // setup the other item links
    m_cv->message_type_case_item = m_cv->case_access->LookupCaseItem(MessageTypeName);
    m_cv->message_number_case_item = m_cv->case_access->LookupCaseItem(MessageNumberName);
    m_cv->message_text_case_item = m_cv->case_access->LookupCaseItem(MessageTextName);
}


void Listing::DataFileLister::WriteMessages(const Messages& messages)
{
    // setup the listing record
    size_t listing_occurrences = std::min(MaxMessagesPerCase, messages.messages.size());
    m_cv->listing_case_record->SetNumberOccurrences(listing_occurrences);
    CaseItemIndex listing_index = m_cv->listing_case_record->GetCaseItemIndex();

    for( const Message& message : messages.messages )
    {
        // copy the level key
        if( !message.level_key.empty() )
        {
            // unfortunately, due to when this method is called, the level key's values
            // will have to be reconstructed from the text of the level key
            const TCHAR* level_key_itr = message.level_key.c_str();
            const TCHAR* level_key_end = level_key_itr + message.level_key.length();

            for( const auto& id_link : m_cv->id_links )
            {
                if( id_link.level_number > 0 && level_key_itr < level_key_end )
                {
                    if( id_link.listing_case_item->IsTypeNumeric() )
                    {
                        assert_cast<const FixedWidthNumericCaseItem&>(*id_link.listing_case_item).SetValueFromTextInput(listing_index, level_key_itr);
                    }

                    else
                    {
                        assert_cast<const FixedWidthStringCaseItem&>(*id_link.listing_case_item).SetFixedWidthValue(listing_index, level_key_itr);
                    }

                    level_key_itr += id_link.listing_case_item->GetDictionaryItem().GetLen();
                }
            }
        }

        // copy the message
        double message_type;
        double message_number;

        if( message.details.has_value() )
        {
            message_type = static_cast<double>(message.details->type);
            message_number = message.details->number;
        }

        else
        {
            message_type = WriteMessageTypeCode;
            message_number = NOTAPPL;
        }

        CaseItemHelpers::SetValue(*m_cv->message_type_case_item, listing_index, message_type);
        CaseItemHelpers::SetValue(*m_cv->message_number_case_item, listing_index, message_number);
        CaseItemHelpers::SetValue(*m_cv->message_text_case_item, listing_index, message.text);

        listing_index.IncrementRecordOccurrence();

        if( listing_index.GetRecordOccurrence() == listing_occurrences )
            break;
    }

    // save the listing case
    m_cv->repository->WriteCase(*m_cv->data_case);
}


void Listing::DataFileLister::ProcessCaseSource(const Case* data_case)
{
    m_cv->data_case->Reset();

    if( data_case == nullptr )
        return;

    // look at the 20220810 note in Exopfile.cpp to see that this can be done in
    // CreateAndInitializeListingDictionary if the lister is created after the CaseAccess object is initialized
    if( m_cv->id_links.front().input_case_item == nullptr )
    {
        ASSERT(m_caseAccess->IsInitialized());

        const auto& case_levels = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
        std::vector<const CaseItem*> id_case_items = case_levels.front()->GetIdCaseRecordMetadata()->GetCaseItems();
        ASSERT(id_case_items.size() == (size_t)std::count_if(m_cv->id_links.cbegin(), m_cv->id_links.cend(),
                                                             [&](const auto& id_link) { return ( id_link.level_number == 0 ); }));

        for( size_t i = 0; i < id_case_items.size(); ++i )
            m_cv->id_links[i].input_case_item = id_case_items[i];
    }

    // copy the root IDs
    CaseItemIndex input_id_index = data_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();
    CaseItemIndex listing_id_index = m_cv->id_case_record->GetCaseItemIndex();

    for( const auto& id_link : m_cv->id_links )
    {
        if( id_link.level_number != 0 )
            break;

        CaseItemHelpers::CopyValue(*id_link.input_case_item, input_id_index, *id_link.listing_case_item, listing_id_index);
    }
}
