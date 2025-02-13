#include "stdafx.h"
#include "SortSpec.h"
#include <zUtilO/Interapp.h>
#include <zUtilO/Specfile.h>
#include <zUtilO/UWM.h>
#include <zJson/JsonSpecFile.h>


constexpr const TCHAR* RecordTypeItemName = _T("CSSORT_RECTYPE");

CREATE_JSON_VALUE_TEXT_OVERRIDE(case_, case)
CREATE_JSON_VALUE(excluded)
CREATE_JSON_VALUE(included)
CREATE_JSON_VALUE(item)
CREATE_JSON_VALUE(record)
CREATE_JSON_VALUE(recordType)
CREATE_JSON_VALUE(sort)


SortSpec::SortSpec()
    :   m_sortType(SortType::Case),
        m_recordSortDictRecord(nullptr)
{
}


SortSpec::~SortSpec()
{
}


void SortSpec::ClearSortItems()
{
    m_recordTypeDictItem.reset();
    m_possibleSortableDictItems.clear();
    m_usedSortItems.clear();
}


void SortSpec::Load(const std::wstring& filename, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/)
{
    std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(filename, nullptr, [&]() { return ConvertPre80SpecFile(filename); });

    try
    {
        json_reader->CheckVersion();
        json_reader->CheckFileType(JV::sort);

        Load(*json_reader, silent, std::move(embedded_dictionary), json_reader->GetSharedMessageLogger());
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(filename, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void SortSpec::Load(const JsonNode<wchar_t>& json_node, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/,
                    std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    ClearSortItems();

    // when run from the engine, the dictionary may be supplied
    if( embedded_dictionary != nullptr )
    {
        m_dictionary = std::move(embedded_dictionary);
    }

    else
    {
        const std::wstring dictionary_filename = json_node.GetAbsolutePath(JK::dictionary);

        if( WindowsDesktopMessage::Send(UWM::UtilO::GetSharedDictionaryConst, &dictionary_filename, &m_dictionary) == 1 )
        {
            ASSERT(m_dictionary != nullptr);
        }

        else
        {
            m_dictionary = CDataDict::InstantiateAndOpen(dictionary_filename, silent, std::move(message_logger));
        }
    }

    // get the sort details
    const bool case_sort = ( json_node.GetOrDefault<wstring_view>(JK::sortType, JV::case_) == JV::case_ );
    const bool use_record_items = json_node.GetOrDefault(JK::useRecordItems, false);

    m_sortType = case_sort ? ( use_record_items ? SortType::CasePlus : SortType::Case ) :
                             SortType::Record;

    if( m_sortType == SortType::Record && json_node.Contains(JK::record) )
    {
        const std::wstring record_name = json_node.Get<std::wstring>(JK::record);
        m_recordSortDictRecord = m_dictionary->FindRecord(record_name);

        if( m_recordSortDictRecord != nullptr )
        {
            m_sortType = SortType::RecordUsing;
        }

        else
        {
            json_node.LogWarning(_T("The record '%s' is not in the dictionary '%s'"),
                                 record_name.c_str(), m_dictionary->GetName().GetString());
        }
    }

    // add the possible sort keys
    RefreshPossibleSortItems();

    // read the keys
    for( const auto& key_node : json_node.GetArrayOrEmpty(JK::keys) )
    {
        const std::wstring item_name = ( key_node.GetOptional<wstring_view>(JK::type) == JV::recordType ) ?
                                       RecordTypeItemName :
                                       key_node.Get<std::wstring>(JK::name);

        const auto& item_lookup = std::find_if(m_possibleSortableDictItems.cbegin(), m_possibleSortableDictItems.cend(),
                                               [&](const CDictItem* dict_item) { return SO::EqualsNoCase(item_name, dict_item->GetName()); });

        if( item_lookup == m_possibleSortableDictItems.cend() )
        {
            json_node.LogWarning(_T("The item '%s' is not a valid sortable item in the dictionary '%s'"),
                                 item_name.c_str(), m_dictionary->GetName().GetString());
            continue;
        }

        const CDictItem* dict_item = *item_lookup;
        const SortOrder sort_order = key_node.GetOrDefault(JK::ascending, true) ? SortOrder::Ascending :
                                                                                  SortOrder::Descending;

        // add the item if it hasn't already been added
        if( dict_item != nullptr && std::find_if(m_usedSortItems.cbegin(), m_usedSortItems.cend(),
                                                 [&](const SortItem& sort_item) { return ( sort_item.dict_item == dict_item ); }) == m_usedSortItems.cend() )
        {
            m_usedSortItems.emplace_back(SortItem { dict_item, sort_order });
        }
    }
}


void SortSpec::Save(const std::wstring& filename) const
{
    ASSERT(m_dictionary != nullptr);

    std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(filename, JV::sort);

    json_writer->WriteRelativePath(JK::dictionary, CS2WS(m_dictionary->GetFullFileName()));

    json_writer->Write(JK::sortType, IsCaseSort() ? JV::case_ : JV::record);

    json_writer->Write(JK::useRecordItems, ( m_sortType == SortType::CasePlus || m_recordSortDictRecord != nullptr ));

    if( m_recordSortDictRecord != nullptr )
        json_writer->Write(JK::record, m_recordSortDictRecord->GetName());

    json_writer->WriteObjects(JK::keys, m_usedSortItems,
        [&](const SortItem& used_sort_item)
        {
            const bool is_record_type = ( used_sort_item.dict_item->GetName() == RecordTypeItemName );

            json_writer->Write(JK::type, is_record_type ? JV::recordType : JV::item);

            if( !is_record_type )
                json_writer->Write(JK::name, used_sort_item.dict_item->GetName());

            json_writer->Write(JK::ascending, ( used_sort_item.order == SortOrder::Ascending ));
        });

    json_writer->EndObject();
}


void SortSpec::RefreshPossibleSortItems()
{
    ClearSortItems();

    // add a fake record type item for record sorts
    if( IsRecordSort() && m_dictionary->GetRecTypeLen() > 0 )
    {
        m_recordTypeDictItem = std::make_unique<CDictItem>();
        m_recordTypeDictItem->SetName(RecordTypeItemName);
        m_recordTypeDictItem->SetLabel(_T("<record type>"));
        m_recordTypeDictItem->SetContentType(ContentType::Alpha);
        m_possibleSortableDictItems.emplace_back(m_recordTypeDictItem.get());
    }


    // add the level IDs
    for( const std::vector<const CDictItem*>& id_dict_items_by_level : m_dictionary->GetIdItemsByLevel() )
    {
        for( const CDictItem* id_dict_item : id_dict_items_by_level )
            m_possibleSortableDictItems.emplace_back(id_dict_item);

        // case sorts only have the primary level IDs
        if( IsCaseSort() )
            break;
    }


    // add singly-occurring items from singly-occurring records on the primary level
    if( m_sortType == SortType::CasePlus )
    {
        const DictLevel& dict_level = m_dictionary->GetLevel(0);

        for( int r = 0; r < dict_level.GetNumRecords(); ++r )
        {
            const CDictRecord* dict_record = dict_level.GetRecord(r);

            if( dict_record->GetMaxRecs() == 1 )
            {
                for( int i = 0; i < dict_record->GetNumItems(); ++i )
                {
                    const CDictItem* dict_item = dict_record->GetItem(i);

                    if( dict_item->GetOccurs() == 1 )
                        m_possibleSortableDictItems.emplace_back(dict_item);
                }
            }
        }
    }


    // add items from the chosen record (for record sorts)
    if( m_recordSortDictRecord != nullptr )
    {
        for( int i = 0; i < m_recordSortDictRecord->GetNumItems(); ++i )
            m_possibleSortableDictItems.emplace_back(m_recordSortDictRecord->GetItem(i));
    }
}


void SortSpec::SetDictionary(std::shared_ptr<const CDataDict> dictionary)
{
    m_dictionary = std::move(dictionary);
    ASSERT(m_dictionary != nullptr);

    RefreshPossibleSortItems();
}


void SortSpec::SetSortType(const SortType sort_type, const CDictRecord* record_sort_dict_record/* = nullptr*/)
{
    ASSERT(( sort_type == SortType::RecordUsing ) == ( record_sort_dict_record != nullptr ));

    m_sortType = sort_type;
    m_recordSortDictRecord = record_sort_dict_record;

    RefreshPossibleSortItems();
}


void SortSpec::SetSortItems(const std::vector<std::tuple<int, SortOrder>>& sort_item_indices_and_orders)
{
    m_usedSortItems.clear();

    for( const auto& [index, order] : sort_item_indices_and_orders )
    {
        ASSERT(static_cast<size_t>(index) < m_possibleSortableDictItems.size());
        m_usedSortItems.emplace_back(SortItem { m_possibleSortableDictItems[index], order });
    }
}


std::wstring SortSpec::ConvertPre80SpecFile(const std::wstring& filename)
{
    CSpecFile specfile;
    
    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the sort specification file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JV::sort);

    try
    {
        CString command;
        CIMSAString argument;

        auto read_header = [&](const TCHAR* header)
        {
            if( !specfile.IsHeaderOK(header) )
                throw CSProException(_T("The heading or section '%s' was missing"), header);
        };

        auto read_line = [&](const TCHAR* command_required = nullptr, const bool allow_end_of_file = false)
        {
            if( specfile.GetLine(command, argument) != SF_OK )
            {
                if( !allow_end_of_file )
                    throw CSProException("The file was not complete");

                return false;
            }

            else if( command_required != nullptr && command.CompareNoCase(command_required) != 0 )
            {
                throw CSProException(_T("The command '%s' was not found"), command_required);
            }

            return true;
        };

        auto read_line_if = [&](const TCHAR* command_required) -> bool
        {
            read_line();

            if( command.CompareNoCase(command_required) == 0 )
            {
                return true;
            }

            else
            {
                specfile.UngetLine();
                return false;
            }
        };


        // is this a correct spec file?
        read_header(_T("[CSSort]"));

        // read the version number (ignoring errors)
        specfile.IsVersionOK(CSPRO_VERSION);


        // get the dictionary filename
        read_header(_T("[Dictionaries]"));
        read_line(_T("File"));

        json_writer->Write(JK::dictionary, specfile.EvaluateRelativeFilename(argument));


        // get the sort type
        if( read_line_if(_T("[SortType]")) )
        {
            bool case_sort;
            bool use_record_items;            

            read_line(_T("Type"));

            if( argument.CompareNoCase(_T("Questionnaire")) == 0 )
            {
                case_sort = true;
                use_record_items = false;
            }

            else if( argument.CompareNoCase(_T("Questionnaire Plus")) == 0 )
            {
                case_sort = true;
                use_record_items = true;
            }

            else if( argument.CompareNoCase(_T("Record")) == 0 )
            {
                case_sort = false;
                use_record_items = read_line_if(_T("Using"));

                if( use_record_items )
                    json_writer->Write(JK::record, static_cast<const CString&>(argument.GetToken()));
            }

            else
            {
                throw CSProException(_T("The sort type '%s' was not valid"), argument.GetString());
            }

            json_writer->Write(JK::sortType, case_sort ? JV::case_ : JV::record);
            json_writer->Write(JK::useRecordItems, use_record_items);
        }


        // read the keys
        read_header(_T("[Keys]"));
        json_writer->BeginArray(JK::keys);

        while( read_line(_T("Key"), true) )
        {
            const CString order_text = argument.GetToken();
            const SortOrder sort_order = ( order_text.CompareNoCase(_T("Ascending")) == 0 )  ? SortOrder::Ascending :
                                         ( order_text.CompareNoCase(_T("Descending")) == 0 ) ? SortOrder::Descending :
                                         throw CSProException(_T("The sort order '%s' was not valid"), order_text.GetString());

            const CString item_name = argument.GetToken();

            json_writer->WriteObject(
                [&]()
                {
                    const bool is_record_type = ( item_name == RecordTypeItemName );

                    json_writer->Write(JK::type, is_record_type ? JV::recordType : JV::item);

                    if( !is_record_type )
                        json_writer->Write(JK::name, item_name);

                    json_writer->Write(JK::ascending, ( sort_order == SortOrder::Ascending ));
                });
        }

        json_writer->EndArray();

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the sort specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    json_writer->EndObject();

    return json_writer->GetString();
}
