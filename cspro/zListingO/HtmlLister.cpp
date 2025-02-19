#include "stdafx.h"
#include "HtmlLister.h"
#include <zToolsO/Special.h>
#include <zUtilO/CustomUri.h>
#include <zHtml/HtmlWriter.h>


// --------------------------------------------------------------------------
// ProcessSummaryHTMLFormatter
// --------------------------------------------------------------------------

class Listing::ProcessSummaryHTMLFormatter
{
public:
    ProcessSummaryHTMLFormatter(const ProcessSummary& process_summary, FileIO::TextFile& text_file, HtmlWriter& html_writer);

    void WriteShell();
    void WriteValues();

private:
    struct LevelSummaryPosition
    {
        std::optional<int64_t> levelNumberPosition;
        std::optional<int64_t> inputCasesPosition;
        std::optional<int64_t> badStructuresPosition;
        std::optional<int64_t> levelPostsPosition;
    };

    const ProcessSummary& m_processSummary;
    FileIO::TextFile& m_textFile;
    HtmlWriter& m_htmlWriter;

    std::optional<int64_t> m_processSummaryPosition;

    std::optional<int64_t> m_attributesReadPosition;
    std::optional<int64_t> m_attributesTypePosition;

    std::optional<int64_t> m_attributesIgnoredPosition;
    std::optional<int64_t> m_attributesUnknownPosition;
    std::optional<int64_t> m_attributesErasedPosition;

    std::optional<int64_t> m_totalMessagePosition;
    std::optional<int64_t> m_errorMessagesPosition;
    std::optional<int64_t> m_warningMessagesPosition;
    std::optional<int64_t> m_userMessagesPosition;

    std::vector<LevelSummaryPosition> m_levelSummaryPositions;
};



Listing::ProcessSummaryHTMLFormatter::ProcessSummaryHTMLFormatter(const ProcessSummary& process_summary, FileIO::TextFile& text_file, HtmlWriter& html_writer)
    :   m_processSummary(process_summary),
        m_textFile(text_file),
        m_htmlWriter(html_writer)
{
}


void Listing::ProcessSummaryHTMLFormatter::WriteShell()
{
    m_processSummaryPosition = m_textFile.FlushAndGetPosition();

    m_htmlWriter << "<table>\n";
    m_htmlWriter << "<thead>\n<tr><th colspan='4' class='center'>CSPro Process Summary</th></tr>\n</thead>\n";

    // write out HTML for the number of records (or slices) read
    m_htmlWriter << "<tr class='highlight'><td class='width-25'>";

    m_htmlWriter << ( ( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records ) ? "Records Read" : "Slices Read" );
    m_htmlWriter << "</td><td class='width-25'>Total Ignored</td><td class='width-25'>Unknown</td><td class='width-25'>Erased</td></tr>\n";

    m_htmlWriter << "<tr><td>";

    m_attributesReadPosition = m_textFile.FlushAndGetPosition();
    m_htmlWriter << SO::GetRepeatingCharacterString(' ', 43);

    m_htmlWriter << "</td><td>";

    m_attributesIgnoredPosition = m_textFile.FlushAndGetPosition();
    m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

    m_htmlWriter << "</td><td>";

    m_attributesUnknownPosition = m_textFile.FlushAndGetPosition();
    m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

    m_htmlWriter << "</td><td>";

    m_attributesErasedPosition = m_textFile.FlushAndGetPosition();
    m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

    m_htmlWriter<< "</td></tr>\n";

    if( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        // write out HTML for level summaries
        m_htmlWriter << "<tbody>\n<tr class='highlight'><td>Level</td><td>Input Case</td>"
                        "<td>Bad Struct</td><td>Level Post</td></tr>\n";

        for( size_t level_number = 0; level_number < m_processSummary.GetNumberLevels(); ++level_number )
        {
            LevelSummaryPosition levelSummaryPosition;

            m_htmlWriter << "<tr><td>";

            levelSummaryPosition.levelNumberPosition = m_textFile.FlushAndGetPosition();
            m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

            m_htmlWriter << "</td><td>";

            levelSummaryPosition.inputCasesPosition = m_textFile.FlushAndGetPosition();
            m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

            m_htmlWriter << "</td><td>";

            levelSummaryPosition.badStructuresPosition = m_textFile.FlushAndGetPosition();
            m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

            m_htmlWriter << "</td><td>";

            levelSummaryPosition.levelPostsPosition = m_textFile.FlushAndGetPosition();
            m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

            m_htmlWriter << "</td></tr>\n";

            m_levelSummaryPositions.emplace_back(std::move(levelSummaryPosition));
        }

        // write out HTML for messages
        m_htmlWriter << "<tr class='highlight'><td>Total Messages</td>"
                        "<td class='error'>Error</td><td class='warning'>Warning</td>"
                        "<td class='user-defined'>User Defined</td></tr>\n";

        m_htmlWriter << "<tr><td>";

        m_totalMessagePosition = m_textFile.FlushAndGetPosition();
        m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

        m_htmlWriter << "</td><td>";

        m_errorMessagesPosition = m_textFile.FlushAndGetPosition();
        m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

        m_htmlWriter << "</td><td>";

        m_warningMessagesPosition = m_textFile.FlushAndGetPosition();
        m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

        m_htmlWriter << "</td><td>";

        m_userMessagesPosition = m_textFile.FlushAndGetPosition();
        m_htmlWriter << SO::GetRepeatingCharacterString(' ', 20);

        m_htmlWriter << "</td></tr>\n</tbody>\n";
    }

    m_htmlWriter << "</table>\n<br>\n";
}


void Listing::ProcessSummaryHTMLFormatter::WriteValues()
{
    // write out values for the number of records (or slices) read
    ASSERT(m_attributesReadPosition.has_value());

    m_textFile.Seek(*m_attributesReadPosition, SEEK_SET);

    std::string recordsRead = FormatText("%d (%d%%)", static_cast<int>(m_processSummary.GetAttributesRead()),
                                                      static_cast<int>(m_processSummary.GetPercentSourceRead()));
    m_htmlWriter.WriteRaw(SO::MakeExactLength(recordsRead, 43));

    // write out values for bad records (or slices)
    ASSERT(m_attributesIgnoredPosition.has_value());
    m_textFile.Seek(*m_attributesIgnoredPosition, SEEK_SET);
    m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetAttributesIgnored())));

    ASSERT(m_attributesUnknownPosition.has_value());
    m_textFile.Seek(*m_attributesUnknownPosition, SEEK_SET);
    m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetAttributesUnknown())));

    ASSERT(m_attributesErasedPosition.has_value());
    m_textFile.Seek(*m_attributesErasedPosition, SEEK_SET);
    m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetAttributesErased())));

    if( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        // write out values for messages
        ASSERT(m_totalMessagePosition.has_value());
        m_textFile.Seek(*m_totalMessagePosition, SEEK_SET);
        m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetTotalMessages())));

        ASSERT(m_errorMessagesPosition.has_value());
        m_textFile.Seek(*m_errorMessagesPosition, SEEK_SET);
        m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetErrorMessages())));

        ASSERT(m_warningMessagesPosition.has_value());
        m_textFile.Seek(*m_warningMessagesPosition, SEEK_SET);
        m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetWarningMessages())));

        ASSERT(m_userMessagesPosition.has_value());
        m_textFile.Seek(*m_userMessagesPosition, SEEK_SET);
        m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetUserMessages())));

        ASSERT(m_levelSummaryPositions.size() == m_processSummary.GetNumberLevels());

        for( size_t level_number = 0; level_number < m_processSummary.GetNumberLevels(); ++level_number )
        {
            // write out the level summaries
            const LevelSummaryPosition& levelSummaryPosition = m_levelSummaryPositions[level_number];

            ASSERT(levelSummaryPosition.levelNumberPosition.has_value());
            m_textFile.Seek(*levelSummaryPosition.levelNumberPosition, SEEK_SET);
            m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(level_number + 1)));

            ASSERT(levelSummaryPosition.inputCasesPosition.has_value());
            m_textFile.Seek(*levelSummaryPosition.inputCasesPosition, SEEK_SET);
            m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetCaseLevelsRead(level_number))));

            ASSERT(levelSummaryPosition.badStructuresPosition.has_value());
            m_textFile.Seek(*levelSummaryPosition.badStructuresPosition, SEEK_SET);
            m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetBadCaseLevelStructures(level_number))));

            ASSERT(levelSummaryPosition.levelPostsPosition.has_value());
            m_textFile.Seek(*levelSummaryPosition.levelPostsPosition, SEEK_SET);
            m_htmlWriter.WriteRaw(FormatText("%*zu", 20, static_cast<unsigned>(m_processSummary.GetLevelPostProcsExecuted(level_number))));
        }
    }

    m_textFile.SeekToEnd();
}



// --------------------------------------------------------------------------
// HtmlLister
// --------------------------------------------------------------------------

Listing::HtmlLister::HtmlLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, const bool append, const PFF& pff)
    :   Lister(std::move(process_summary)),
        m_writeProcessSummaryAndMessages(pff.GetAppType() != APPTYPE::ENTRY_TYPE),
        m_isProcessMessageComplete(false),
        m_isMultiLevel(false)
{
    m_hasData = ( append && PortableFunctions::FileSize(UTF8_TODO::GetWide(file_path)) > TextEncoding::Utf8Bom_sv.length() ); // TEXT_ENCODING_TODO review

    m_textFile = OpenListingFile(file_path, append);
    m_htmlWriter = std::make_unique<HtmlWriter>(m_textFile->GetOutputStream());
}


Listing::HtmlLister::~HtmlLister()
{
}


void Listing::HtmlLister::WriteHeader(const std::vector<HeaderAttribute>& header_attributes)
{
    if( m_hasData )
    {
        MoveToHtmlEndTag();
    }

    else
    {
        m_htmlWriter->WriteDefaultHeader("HTML Listing", Html::CSS::Common);
        *m_htmlWriter << "\n<body class='container-page'>\n";
    }

    *m_htmlWriter << "<table>\n";
    *m_htmlWriter << "<thead>\n<tr><th colspan='2' class='center'>Listing Details</th></tr>\n</thead>\n";

    // write out the header attributes
    for( const HeaderAttribute& header_attribute : header_attributes )
    {
        *m_htmlWriter << "<tr>";

        *m_htmlWriter << "<td class='left width-10'>" << header_attribute.description << "</td>";

        std::string secondary_description;

        if( header_attribute.secondary_description.has_value() )
        {
            secondary_description = *header_attribute.secondary_description + "=";
        }

        if( std::holds_alternative<std::string>(header_attribute.value) )
        {
            *m_htmlWriter << "<td class='left'>" << secondary_description << std::get<std::string>(header_attribute.value) << "</td>";
        }

        else
        {
            const ConnectionString& connection_string = std::get<ConnectionString>(header_attribute.value);
            const std::optional<std::string> data_uri = CreateDataUri(connection_string, *header_attribute.dictionary);

            *m_htmlWriter << "<td class='left'>" << secondary_description;

            if( data_uri.has_value() )
            {
                *m_htmlWriter << "<a href=\"";
                m_htmlWriter->WriteTagValue(*data_uri);
                *m_htmlWriter  << "\">";
            }

            *m_htmlWriter << connection_string.GetName(DataRepositoryNameType::ForListing);

            if( data_uri.has_value() )
                *m_htmlWriter << "</a>";

            *m_htmlWriter << "</td>";
        }

        *m_htmlWriter << "</tr>\n";
    }

    // write out the date
    *m_htmlWriter << "<tr><td class='left'>Date</td><td class='left'>" << DateTime::LocalDateString() << "</td></tr>\n";

    const std::string time = DateTime::LocalTimeString();
    *m_htmlWriter << "<tr><td class='left'>Start Time</td><td class='left'>" << time << "</td></tr>\n";
    *m_htmlWriter << "<tr><td class='left'>End Time</td><td class='left'>" << time;

    m_endTimePosition = m_textFile->FlushAndGetPosition() - time.length();

    *m_htmlWriter << "</td></tr>\n</table>\n<br>\n";

    if( !m_writeProcessSummaryAndMessages )
        return;

    m_processSummaryFormatter = std::make_unique<ProcessSummaryHTMLFormatter>(*m_processSummary, *m_textFile, *m_htmlWriter);
    m_processSummaryFormatter->WriteShell();

    m_startProcessMessagePosition = m_textFile->FlushAndGetPosition();
    m_textFile->WriteFormattedLine(SO::GetRepeatingCharacterString(' ', 110));
}


void Listing::HtmlLister::WriteMessages(const Messages& messages)
{
    constexpr const char* TypeCodes[] =
    {
        "A",
        "E",
        "W",
        "U"
    };

    // write the message source
    *m_htmlWriter << "<tr class='highlight'>";

    const char* const colspan_size = m_isMultiLevel ? "3" : "2";

    if( m_caseKeyUuid.has_value() )
    {
        // construct the full link to open the case in Data Manager
        const std::optional<std::string> case_uri = CreateCaseUri(m_inputDataUri, m_caseKeyUuid);

        *m_htmlWriter << "<td colspan='" << colspan_size << "' class='left'>Case [";

        if( case_uri.has_value() )
        {
            *m_htmlWriter << "<a href=\"";
            m_htmlWriter->WriteTagValue(*case_uri);
            *m_htmlWriter << "\">";
        }

        *m_htmlWriter << messages.source;

        if( case_uri.has_value() )
            *m_htmlWriter << "</a>";

        *m_htmlWriter << "]</td>";
    }

    else
    {
        *m_htmlWriter << "<td colspan='" << colspan_size << "' class='left'>" << messages.source << "</td>";
    }

    *m_htmlWriter << "</tr>\n";

    // write the messages
    for( const Message& message : messages.messages )
    {
        // write a message
        if( message.details.has_value() )
        {
            ASSERT(static_cast<size_t>(message.details->type) < _countof(TypeCodes));
            const char* const type_code = TypeCodes[static_cast<size_t>(message.details->type)];

            const char* const message_style = ( message.details->type == MessageType::Error )   ? "error" :
                                              ( message.details->type == MessageType::Warning ) ? "warning" :
                                              ( message.details->type == MessageType::User )    ? "user-defined" :
                                                                                                  "";

            *m_htmlWriter << "<tr>";

            if( m_isMultiLevel )
            {
                *m_htmlWriter << "<td class='left width-10'>";

                if( !message.level_key.empty() )
                    *m_htmlWriter << "[" << message.level_key << "]";

                 *m_htmlWriter << "</td>";
            }

            *m_htmlWriter << "<td class='" << message_style << " left width-10'>" << type_code << ""
                          << IntToString(message.details->number)
                          << "</td><td class='left'>" << *message.text << "</td></tr>\n";
        }

        else
        {
            // there is no special formatting for text coming from the write function
            *m_htmlWriter << "<tr><td class='left'>" << *message.text << "</td></tr>\n";
        }
    }
}


void Listing::HtmlLister::ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    m_inputDataUri = CreateDataUri(connection_string, dictionary);
    m_isMultiLevel = ( dictionary.GetNumLevels() > 1 );
}


void Listing::HtmlLister::ProcessCaseSource(const Case* const data_case)
{
    if( data_case == nullptr )
    {
        m_caseKeyUuid.reset();
    }

    else
    {
        m_caseKeyUuid.emplace(data_case->GetKey(), data_case->GetUuid());
    }
}


void Listing::HtmlLister::WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries)
{
    if( m_writeProcessSummaryAndMessages )
        WriteUpdatesToProcessMessageTable();

    ASSERT(!message_summaries.empty());

    const MessageSummary::Type message_summaries_type = message_summaries.front().type;
    const char* const type_text = ( message_summaries_type == MessageSummary::Type::System )       ? "System" :
                                  ( message_summaries_type == MessageSummary::Type::UserNumbered ) ? "User Numbered" :
                                                                                                     "User Unnumbered";

    // write the header
    *m_htmlWriter << "<br><br>\n<table>\n";

    const char* const colspan_size = ( message_summaries_type == MessageSummary::Type::System ) ? "3" : "5";

    *m_htmlWriter << "<thead>\n<tr><th colspan='" << colspan_size << "' class='center'>"
                  << type_text << " Messages</th></tr>\n</thead>\n";

    const bool use_denoms = ( message_summaries_type != MessageSummary::Type::System );
    const char* const number_heading = ( message_summaries_type == MessageSummary::Type::UserUnnumbered ) ? "Line" : "Number";
    constexpr const char* frequency_heading = "Freq";
    constexpr const char* percent_heading = "%";
    constexpr size_t percent_heading_length = std::string_view(percent_heading).length();
    constexpr const char* message_heading = "Message Text";
    constexpr const char* denom_heading = "Denom";
    constexpr size_t denom_heading_length = std::string_view(denom_heading).length();

    *m_htmlWriter << "<tr class='highlight'><td class='width-10'>" << number_heading << "</td><td class='width-10'>" << frequency_heading << "</td>";

    if( use_denoms )
        *m_htmlWriter << "<td class='width-10'>" << percent_heading << "</td>";

    *m_htmlWriter << "<td class='left'>" << message_heading << "</td>";

    if( use_denoms )
        *m_htmlWriter << "<td class='width-10'>" << denom_heading << "</td>";

    *m_htmlWriter << "</tr>\n";

    // write the messages
    constexpr const char* blank_percent_denom_text = "-";
    std::string percent_text;
    std::string denom_text;

    for( const MessageSummary& message_summary : message_summaries )
    {
        const bool has_a_denom = ( use_denoms && message_summary.denominator.has_value() );
        const bool has_a_blank_denom = ( use_denoms && !message_summary.denominator.has_value() );

        if( has_a_denom )
        {
            bool calculate_percent = false;

            if( *message_summary.denominator < 0 || IsSpecial(*message_summary.denominator) )
            {
                denom_text = SO::GetRepeatingCharacterString('*', denom_heading_length);
            }

            else
            {
                denom_text = IntToString(static_cast<uint64_t>(*message_summary.denominator));

                if( *message_summary.denominator != 0 && message_summary.frequency <= *message_summary.denominator )
                    calculate_percent = true;
            }

            if( calculate_percent )
            {
                percent_text = FormatText("%5.1f", CreatePercent<double>(message_summary.frequency, *message_summary.denominator));
            }

            else
            {
                percent_text = SO::GetRepeatingCharacterString('*', percent_heading_length);
            }
        }

        // show unnumbered messages' line numbers as positive (rather than negative as they appear elsewhere)
        const int message_number_for_display = std::abs(message_summary.message_number);

        *m_htmlWriter << "<tr><td>" << IntToString(message_number_for_display)
                      << "</td><td>" << IntToString(message_summary.frequency) << "</td>";

        if( has_a_denom )
        {
            *m_htmlWriter << "<td>" << percent_text << "</td>";
        }

        else if( has_a_blank_denom )
        {
            *m_htmlWriter << "<td>" << blank_percent_denom_text << "</td>";
        }

        *m_htmlWriter << "<td class='left'>" << *message_summary.message_text << "</td>";

        if( has_a_denom )
        {
            *m_htmlWriter << "<td>" << denom_text << "</td>";
        }

        else if( has_a_blank_denom )
        {
            *m_htmlWriter << "<td>" << blank_percent_denom_text << "</td>";
        }

        *m_htmlWriter << "</tr>\n";
    }

    *m_htmlWriter << "</table>\n";
}


void Listing::HtmlLister::WriteWarningAboutApplicationErrors(const std::string& application_errors_path)
{
    *m_htmlWriter << "<br>\n<div class='center'><em>"
                     "A compilation error file "
                     "<a href=\"";
    m_htmlWriter->WriteTagValue(CustomUri::CreateTextUri(application_errors_path));
    *m_htmlWriter << "\">"
                     "(" << application_errors_path << ")</a> has been created for your application."
                  << "</em></div>\n";
}


void Listing::HtmlLister::UpdateProcessSummary()
{
    if( m_processSummaryFormatter != nullptr )
        m_processSummaryFormatter->WriteValues();
}


void Listing::HtmlLister::WriteFooter()
{
    *m_htmlWriter << "<br>\n<div class='center'>CSPro Executor Normal End</div>\n<br>\n";
    *m_htmlWriter << "<hr>\n";
    *m_htmlWriter << "</body>\n</html>\n";

    // update the end time
    if( m_endTimePosition.has_value() )
    {
        m_textFile->Seek(*m_endTimePosition, SEEK_SET);
        m_textFile->WriteString(DateTime::LocalTimeString());
        m_textFile->SeekToEnd();
    }
}


void Listing::HtmlLister::WriteUpdatesToProcessMessageTable()
{
    if( m_isProcessMessageComplete )
        return;

    m_isProcessMessageComplete = true;

    const char* const colspan_size = m_isMultiLevel ? "3" : "2";

    m_textFile->Seek(*m_startProcessMessagePosition, SEEK_SET);
    m_textFile->WriteFormattedString("<br><table><thead><tr><th colspan='%s' class='center'>Process Messages</th></tr></thead>", colspan_size);

    m_textFile->SeekToEnd();
    m_textFile->WriteString("</table>\n");
}


void Listing::HtmlLister::MoveToHtmlEndTag() const
{
    // use length of trailing body and html tags to calculate offset
    static_assert(FileIO::TextFile::DefaultWriteNewlineAsCRLF == true);
    constexpr int64_t offset = -1 * static_cast<int64_t>(std::string_view("</body>\r\n</html>\r\n").length());

    m_textFile->Seek(offset, SEEK_END);
}
