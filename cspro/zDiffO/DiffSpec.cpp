#include "stdafx.h"
#include "DiffSpec.h"
#include <zUtilO/Interapp.h>
#include <zUtilO/Specfile.h>
#include <zUtilO/UWM.h>
#include <zJson/JsonSpecFile.h>
#include <zDictO/DictionaryIterator.h>


CREATE_JSON_VALUE(bothWays)
CREATE_JSON_VALUE(compare)
CREATE_JSON_VALUE(indexed)
CREATE_JSON_VALUE(oneWay)
CREATE_JSON_VALUE(sequential)

CREATE_ENUM_JSON_SERIALIZER(DiffSpec::DiffMethod,
    { DiffSpec::DiffMethod::OneWay,   JV::oneWay },
    { DiffSpec::DiffMethod::BothWays, JV::bothWays })

CREATE_ENUM_JSON_SERIALIZER(DiffSpec::DiffOrder,
    { DiffSpec::DiffOrder::Indexed,    JV::indexed },
    { DiffSpec::DiffOrder::Sequential, JV::sequential })

CREATE_ENUM_JSON_SERIALIZER(DiffSpec::ItemDisplay,
    { DiffSpec::ItemDisplay::Labels, _T("labels") },
    { DiffSpec::ItemDisplay::Names,  _T("names") })

CREATE_ENUM_JSON_SERIALIZER(DiffSpec::ItemSerialization,
    { DiffSpec::ItemSerialization::Included, _T("included") },
    { DiffSpec::ItemSerialization::Excluded, _T("excluded") })



DiffSpec::DiffSpec()
    :   m_diffMethod(DiffMethod::OneWay),
        m_diffOrder(DiffOrder::Indexed),
        m_itemDisplay(ItemDisplay::Labels),
        m_itemSerialization(ItemSerialization::Included)
{
}


std::vector<std::tuple<const CDictItem*, std::optional<size_t>>> DiffSpec::GetSortedItemsAndOccurrences() const
{
    // populate all items, including (or excluding) those selected
    class SortedItemsAndOccurrencesIterator : public DictionaryIterator::Iterator
    {
    public:
        SortedItemsAndOccurrencesIterator(const DiffSpec& diff_spec)
            :   m_diffSpec(diff_spec)
        {
        }

        std::vector<std::tuple<const CDictItem*, std::optional<size_t>>> GetSortedItemsAndOccurrences()
        {
            return m_sortedItemsAndOccurrences;
        }

    protected:
        void ProcessItem(CDictItem& dict_item) override
        {
            // skip ID items
            if( m_currentRecord->GetSonNumber() < 0 )
                return;

            for( size_t i = 0; i < dict_item.GetItemSubitemOccurs(); ++i )
            {
                std::tuple<const CDictItem*, std::optional<size_t>> item_and_occurrence(&dict_item,
                    ( dict_item.GetItemSubitemOccurs() > 1 ) ? std::make_optional(i) : std::nullopt);

                if( m_diffSpec.IsItemSelected(item_and_occurrence) != m_diffSpec.GetSaveExcludedItems() )
                    m_sortedItemsAndOccurrences.emplace_back(std::move(item_and_occurrence));
            }
        }

    private:
        const DiffSpec& m_diffSpec;
        std::vector<std::tuple<const CDictItem*, std::optional<size_t>>> m_sortedItemsAndOccurrences;
    };

    SortedItemsAndOccurrencesIterator sorted_items_and_occurrences_iterator(*this);
    sorted_items_and_occurrences_iterator.Iterate(const_cast<CDataDict&>(*m_dictionary));

    return sorted_items_and_occurrences_iterator.GetSortedItemsAndOccurrences();
}


void DiffSpec::Load(const std::wstring& filename, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/)
{
    std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(filename, nullptr, [&]() { return ConvertPre80SpecFile(filename); });

    try
    {
        json_reader->CheckVersion();
        json_reader->CheckFileType(JV::compare);

        Load(*json_reader, silent, std::move(embedded_dictionary), json_reader->GetSharedMessageLogger());
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(filename, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void DiffSpec::Load(const JsonNode<wchar_t>& json_node, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/,
                    std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    m_selectedItemsAndOccurrences.clear();

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

    // get the comparison attributes
    const auto& comparison_node = json_node.GetOrEmpty(JK::comparison);
    m_diffMethod = comparison_node.GetOrDefault(JK::method, m_diffMethod);
    m_diffOrder = comparison_node.GetOrDefault(JK::order, m_diffOrder);

    m_itemDisplay = json_node.GetOrDefault(JK::itemDisplay, m_itemDisplay);
    m_itemSerialization = json_node.GetOrDefault(JK::itemSerialization, m_itemSerialization);

    // read the items
    for( const auto& item_node : json_node.GetArrayOrEmpty(JK::items) )
    {
        const std::wstring item_name = item_node.Get<std::wstring>(JK::name);
        const CDictItem* dict_item = m_dictionary->FindItem(item_name);

        // ignore unknown items
        if( dict_item == nullptr )
        {
            json_node.LogWarning(_T("The item '%s' is not in the dictionary '%s'."),
                                 item_name.c_str(), m_dictionary->GetName().GetString());
            continue;
        }

        std::optional<size_t> occurrence;

        if( dict_item->GetItemSubitemOccurs() > 1 )
        {
            occurrence = item_node.GetOptional<size_t>(JK::occurrence);

            // ignore items with invalid occurrences
            if( !occurrence.has_value() || *occurrence > dict_item->GetItemSubitemOccurs() )
            {
                json_node.LogWarning(_T("The item '%s' is missing an occurrence number."), item_name.c_str());
                continue;
            }

            // occurrences are serialized as one-based
            --(*occurrence);
        }

        m_selectedItemsAndOccurrences.emplace_back(dict_item, occurrence);
    }

    // if loading excluded items, GetSortedItemsAndOccurrences will invert the list of items included above
    if( GetSaveExcludedItems() )
        m_selectedItemsAndOccurrences = GetSortedItemsAndOccurrences();
}


void DiffSpec::Save(const std::wstring& filename) const
{
    ASSERT(m_dictionary != nullptr);

    std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(filename, JV::compare);

    json_writer->WriteRelativePath(JK::dictionary, CS2WS(m_dictionary->GetFullFileName()));

    json_writer->BeginObject(JK::comparison)
                .Write(JK::method, m_diffMethod)
                .Write(JK::order, m_diffOrder)
                .EndObject();

    json_writer->Write(JK::itemDisplay, m_itemDisplay)
                .Write(JK::itemSerialization, m_itemSerialization);

    json_writer->WriteObjects(JK::items, GetSortedItemsAndOccurrences(),
        [&](const std::tuple<const CDictItem*, std::optional<size_t>>& item_and_occurrence)
        {
            json_writer->Write(JK::name, std::get<0>(item_and_occurrence)->GetName());

            // occurrences are serialized as one-based
            if( std::get<1>(item_and_occurrence).has_value() )
                json_writer->Write(JK::occurrence, *std::get<1>(item_and_occurrence) + 1);
        });

    json_writer->EndObject();
}


void DiffSpec::SetDictionary(std::shared_ptr<const CDataDict> dictionary)
{
    m_dictionary = std::move(dictionary);
    ASSERT(m_dictionary != nullptr);

    m_selectedItemsAndOccurrences.clear();
}


bool DiffSpec::IsItemSelected(const std::tuple<const CDictItem*, std::optional<size_t>>& item_and_occurrence) const
{
    return ( std::find(m_selectedItemsAndOccurrences.cbegin(),
                       m_selectedItemsAndOccurrences.cend(),
                       item_and_occurrence) != m_selectedItemsAndOccurrences.cend() );
}


void DiffSpec::SetItemSelection(const std::tuple<const CDictItem*, std::optional<size_t>>& item_and_occurrence, const bool selection)
{
    for( auto itr = m_selectedItemsAndOccurrences.cbegin(); itr != m_selectedItemsAndOccurrences.cend(); ++itr )
    {
        if( item_and_occurrence == *itr )
        {
            if( !selection )
                m_selectedItemsAndOccurrences.erase(itr);

            return;
        }            
    }

    // if a new entry, add it to the end (the list will be sorted in dictionary order before saving)
    if( selection )
        m_selectedItemsAndOccurrences.emplace_back(item_and_occurrence);
}


std::wstring DiffSpec::ConvertPre80SpecFile(const std::wstring& filename)
{
    CSpecFile specfile;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the Compare Data specification file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JV::compare);

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
        read_header(_T("[CSDiff]"));

        // read the version number (ignoring errors)
        specfile.IsVersionOK(CSPRO_VERSION);


        // get the dictionary filename
        read_header(_T("[Dictionaries]"));
        read_line(_T("File"));

        json_writer->Write(JK::dictionary, specfile.EvaluateRelativeFilename(argument));


        // get the comparison attributes
        if( read_line_if(_T("[CompareType]")) )
        {
            json_writer->BeginObject(JK::comparison);

            read_line(_T("Method"));
            json_writer->Write(JK::method, ( argument.CompareNoCase(_T("OneWay")) == 0 ) ? JV::oneWay : JV::bothWays);

            read_line(_T("Order"));
            json_writer->Write(JK::order, ( argument.CompareNoCase(_T("Indexed")) == 0 ) ? JV::indexed : JV::sequential);

            json_writer->EndObject();

            read_line(_T("ReportLabels"));
            json_writer->Write(JK::itemDisplay, ( argument.CompareNoCase(_T("Labels")) == 0 ) ? ItemDisplay::Labels : ItemDisplay::Names);

            read_line(_T("ItemsAre"));
            json_writer->Write(JK::itemSerialization, ( argument.CompareNoCase(_T("Excluded")) == 0 ) ? ItemSerialization::Excluded : ItemSerialization::Included);
        }


        // read the items, ignoring errors
        read_header(_T("[Items]"));
        json_writer->BeginArray(JK::items);

        while( read_line(_T("Item"), true) )
        {
            CString item_name = argument.GetToken();
            CString occurrence_text = argument.GetToken();

            json_writer->WriteObject(
                [&]()
                {
                    json_writer->Write(JK::name, item_name);

                    if( !occurrence_text.IsEmpty() )
                        json_writer->Write(JK::occurrence, occurrence_text);
                });
        }

        json_writer->EndArray();

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the Compare Data specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    json_writer->EndObject();

    return json_writer->GetString();
}
