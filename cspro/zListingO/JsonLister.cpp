#include "stdafx.h"
#include "JsonLister.h"
#include <zJson/Json.h>
#include <zCaseO/CaseItemJsonWriter.h>


// --------------------------------------------------
// serialization routines
// --------------------------------------------------

struct SerializableTimestamp { double timestamp; };
struct SerializableDuration  { double duration;  }; // duration in seconds


CREATE_ENUM_JSON_SERIALIZER(MessageType,
    { MessageType::Abort,   _T("abort") },
    { MessageType::Error,   _T("error") },
    { MessageType::Warning, _T("warning") },
    { MessageType::User,    _T("user") })

CREATE_ENUM_JSON_SERIALIZER(MessageSummary::Type,
    { MessageSummary::Type::System,         _T("system") },
    { MessageSummary::Type::UserNumbered,   _T("user") },
    { MessageSummary::Type::UserUnnumbered, _T("user") })


template<>
struct JsonSerializer<SerializableTimestamp>
{
    static void WriteJson(JsonWriter& json_writer, const SerializableTimestamp& serializable_timestamp)
    {
        time_t tm = static_cast<time_t>(serializable_timestamp.timestamp);
        int year, month, day, hour, minute, second;
        TmToReadableTime(localtime(&tm), &year, &month, &day, &hour, &minute, &second);

        json_writer.BeginObject()
                   .WriteDate(JK::date, tm)
                   .Write(JK::timestamp, serializable_timestamp.timestamp)
                   .Write(JK::year, year)
                   .Write(JK::month, month)
                   .Write(JK::day, day)
                   .Write(JK::hour, hour)
                   .Write(JK::minute, minute)
                   .Write(JK::second, second)
                   .EndObject();
    }
};


template<>
struct JsonSerializer<SerializableDuration>
{
    static void WriteJson(JsonWriter& json_writer, const SerializableDuration& serializable_duration)
    {
        ASSERT(serializable_duration.duration >= 0 );
        const uint64_t duration_in_seconds = static_cast<uint64_t>(serializable_duration.duration);

        json_writer.BeginObject()
                   .Write(JK::totalSeconds, serializable_duration.duration)
                   .Write(JK::hours, duration_in_seconds / 3600)
                   .Write(JK::minutes, ( duration_in_seconds / 60 ) % 60)
                   .Write(JK::seconds, duration_in_seconds % 60)
                   .EndObject();
    }
};


template<>
struct JsonSerializer<Listing::HeaderAttribute>
{
    static void WriteJson(JsonWriter& json_writer, const Listing::HeaderAttribute& header_attribute)
    {
        json_writer.BeginObject();

        // descriptions
        json_writer.Write(JK::description, header_attribute.description);

        if( header_attribute.secondary_description.has_value() )
            json_writer.Write(JK::subdescription, *header_attribute.secondary_description);

        // value
        json_writer.Key(JK::value);

        if( std::holds_alternative<std::wstring>(header_attribute.value) )
        {
            if( PortableFunctions::FileExists(std::get<std::wstring>(header_attribute.value)) )
            {
                json_writer.WritePath(std::get<std::wstring>(header_attribute.value));
            }

            else
            {
                json_writer.Write(std::get<std::wstring>(header_attribute.value));
            }
        }

        else
        {
            std::get<ConnectionString>(header_attribute.value).WriteJson(json_writer, false);
        }

        // dictionary
        if( header_attribute.dictionary != nullptr )
        {
            json_writer.BeginObject(JK::dictionary)
                       .Write(JK::name, header_attribute.dictionary->GetName())
                       .WritePath(JK::path, CS2WS(header_attribute.dictionary->GetFullFileName()))
                       .EndObject();
        }

        json_writer.EndObject();
    }
};


template<>
struct JsonSerializer<MessageSummary>
{
    static void WriteJson(JsonWriter& json_writer, const MessageSummary& message_summary)
    {
        json_writer.BeginObject()
                   .Write(JK::type, message_summary.type)
                   .Write(JK::number, message_summary.message_number)
                   .Write(JK::text, message_summary.message_text)
                   .Write(JK::frequency, message_summary.frequency);

        if( message_summary.denominator.has_value() )
        {
            json_writer.Write(JK::denominator, *message_summary.denominator);

            if( *message_summary.denominator > 0 && !IsSpecial(*message_summary.denominator) )
                json_writer.Write(JK::percent, CreatePercent<double>(message_summary.frequency, *message_summary.denominator));
        }

        json_writer.EndObject();
    }
};



// --------------------------------------------------
// JsonLister
// --------------------------------------------------

Listing::JsonLister::JsonLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, std::shared_ptr<const CaseAccess> case_access)
    :   Lister(std::move(process_summary)),
        m_jsonWriter(Json::CreateFileWriter(filename)),
        m_mustEndListingArray(false),
        m_mustEndMessageSummariesArray(false),
        m_caseAccess(std::move(case_access)),
        m_startTimestamp(GetTimestamp())
{
    // start the (file) object
    m_jsonWriter->BeginObject();
}


Listing::JsonLister::~JsonLister()
{
}


void Listing::JsonLister::WriteHeader(const std::vector<HeaderAttribute>& header_attributes)
{
    m_jsonWriter->Write(JK::attributes, header_attributes);

    // start the listing array
    m_jsonWriter->BeginArray(JK::listing);
    m_mustEndListingArray = true;
}


void Listing::JsonLister::WriteMessages(const Messages& messages)
{
    m_jsonWriter->BeginObject()
                 .Write(JK::source, messages.source);

    // key
    if( !m_keyValues.empty() )
    {
        m_jsonWriter->BeginObject(JK::key);

        for( const auto& [case_item, value] : m_keyValues )
        {
            m_jsonWriter->Key(case_item->GetDictionaryItem().GetName());

            if( case_item->IsTypeNumeric() )
            {
                ASSERT(std::holds_alternative<double>(value));
                CaseItemJsonWriter::WriteCaseItemCode(*m_jsonWriter, assert_cast<const NumericCaseItem&>(*case_item), std::get<double>(value));
            }

            else
            {
                ASSERT(std::holds_alternative<std::wstring>(value));
                CaseItemJsonWriter::WriteCaseItemCode(*m_jsonWriter, assert_cast<const StringCaseItem&>(*case_item), std::get<std::wstring>(value));
            }
        }

        m_jsonWriter->EndObject();
    }

    // messages
    {
        m_jsonWriter->WriteObjects(JK::messages, messages.messages,
            [&](const Message& message)
            {
                m_jsonWriter->WriteIfNotBlank(JK::levelKey, message.level_key);

                if( message.details.has_value() )
                {
                    m_jsonWriter->Write(JK::type, message.details->type);
                    m_jsonWriter->Write(JK::number, message.details->number);
                }

                m_jsonWriter->Write(JK::text, message.text);
            });
    }

    // frequencies
    if( !m_jsonFrequencyTexts.empty() )
        WriteFrequencies();

    m_jsonWriter->EndObject();
}


void Listing::JsonLister::WriteFrequencies()
{
    ASSERT(!m_jsonFrequencyTexts.empty());

    m_jsonWriter->BeginArray(JK::frequencies);

    try
    {
        for( const std::string& json_frequency_text : m_jsonFrequencyTexts )
        {
            const auto frequencies_array_node = Json::Parse(json_frequency_text);

            for( const auto& json_node : frequencies_array_node.GetArray() )
                m_jsonWriter->Write(json_node);
        }
    }

    catch( const JsonParseException& )
    {
        ASSERT(false);
    }

    m_jsonWriter->EndArray();

    m_jsonFrequencyTexts.clear();
}


void Listing::JsonLister::ProcessCaseSource(const Case* data_case)
{
    // look at the 20220810 note in Exopfile.cpp to see that this can be done in the 
    // constructor if the lister is created after the CaseAccess object is initialized
    if( m_idCaseItems.empty() && data_case != nullptr && m_caseAccess != nullptr )
    {
        ASSERT(m_caseAccess->IsInitialized());

        // get the case items for the key
        const auto& case_levels = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
        m_idCaseItems = case_levels.front()->GetIdCaseRecordMetadata()->GetCaseItems();

        // get the level names
        m_levelNames = std::make_unique<std::vector<std::wstring>>();

        for( const CaseLevelMetadata* case_level_metadata : case_levels )
            m_levelNames->emplace_back(case_level_metadata->GetDictLevel().GetName());
    }

    if( m_idCaseItems.empty() )
        return;

    m_keyValues.clear();

    if( data_case == nullptr )
        return;

    CaseItemIndex index = data_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();

    for( const CaseItem* case_item : m_idCaseItems )
    {
        ASSERT(case_item->IsTypeFixed());

        if( case_item->IsBlank(index) )
            continue;

        if( case_item->IsTypeNumeric() )
        {
            m_keyValues.emplace_back(case_item, assert_cast<const NumericCaseItem&>(*case_item).GetValueForOutput(index));
        }

        else
        {
            ASSERT(case_item->IsTypeString());
            m_keyValues.emplace_back(case_item, CS2WS(assert_cast<const StringCaseItem&>(*case_item).GetValue(index)));
        }
    }
}


void Listing::JsonLister::EndListingArrayAndStartSummaryObjectIfNecessary()
{
    if( m_mustEndListingArray )
    {
        // if there were no messages issued but unnamed frequencies were printed, add them here
        if( !m_jsonFrequencyTexts.empty() )
        {
            m_jsonWriter->BeginObject()
                         .Write(JK::source, _T(""));

            WriteFrequencies();

            m_jsonWriter->EndObject();
        }

        // end the listing array
        m_jsonWriter->EndArray();
        m_mustEndListingArray = false;

        // start the summary object
        m_jsonWriter->BeginObject(JK::summary);
    }
}


void Listing::JsonLister::WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries)
{
    EndListingArrayAndStartSummaryObjectIfNecessary();

    if( !m_mustEndMessageSummariesArray )
    {
        m_jsonWriter->BeginArray(JK::messages);
        m_mustEndMessageSummariesArray = true;
    }

    for( const MessageSummary& message_summary : message_summaries )
        m_jsonWriter->Write(message_summary);
}


void Listing::JsonLister::WriteFooter()
{
    EndListingArrayAndStartSummaryObjectIfNecessary();

    if( m_mustEndMessageSummariesArray )
        m_jsonWriter->EndArray();

    // write the process summary
    {
        ASSERT(m_processSummary != nullptr);
        m_jsonWriter->Key(JK::process);
        m_processSummary->WriteJson(*m_jsonWriter, m_levelNames.get());
    }

    // write the start/end times and duration
    {
        SerializableTimestamp start_timestamp { m_startTimestamp };
        SerializableTimestamp end_timestamp { GetTimestamp() };
        SerializableDuration duration { end_timestamp.timestamp - start_timestamp.timestamp };

        m_jsonWriter->BeginObject(JK::runtime)
                     .Write(JK::start, start_timestamp)
                     .Write(JK::end, end_timestamp)
                     .Write(JK::duration, duration)
                     .EndObject();
    }

    // end the summary object
    m_jsonWriter->EndObject();

    // end the (file) object
    m_jsonWriter->EndObject();
}


std::optional<std::tuple<bool, Listing::ListingType, void*>> Listing::JsonLister::GetFrequencyPrinter()
{
    return std::tuple<bool, Listing::ListingType, void*>(false, ListingType::Json, &m_jsonFrequencyTexts.emplace_back());
}
