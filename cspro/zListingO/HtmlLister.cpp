#include "stdafx.h"
#include "HtmlLister.h"
#include <zToolsO/Special.h>
#include <zHtml/HtmlWriter.h>


namespace Listing
{
    class ProcessSummaryHTMLFormatter
    {
    private:
        struct LevelSummaryPosition
        {
            std::optional<ULONGLONG> levelNumberPosition;
            std::optional<ULONGLONG> inputCasesPosition;
            std::optional<ULONGLONG> badStructuresPosition;
            std::optional<ULONGLONG> levelPostsPosition;
        };

    public:
        ProcessSummaryHTMLFormatter(std::shared_ptr<ProcessSummary> process_summary, CStdioFileUnicode& file)
            :   m_processSummary(std::move(process_summary)),
                m_file(file)
        {
        }

        void WriteShell()
        {
            m_processSummaryPosition = m_file.FlushAndGetPosition();

            HtmlStringWriter html_writer;
            html_writer << L"<table>\n";
            html_writer << L"<thead>\n<tr><th colspan='4' class='center'>CSPro Process Summary</th></tr>\n</thead>\n";

            // write out HTML for the number of records (or slices) read
            html_writer << L"<tr class='highlight'><td class='width-25'>";
            m_file.WriteString(html_writer.str());
            html_writer.clear();
            // trailing space in "slices read " is intentional, so strings are equal length
            std::wstring attrType = m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records ? _T("Records Read") : _T("Slices Read ");
            m_file.WriteFormattedString(_T("%*s %s"), 20, L"", attrType.c_str());
            html_writer << L"</td><td class='width-25'>Total Ignored</td><td class='width-25'>Unknown</td><td class='width-25'>Erased</td></tr>\n";

            html_writer << L"<tr><td>";
            m_file.WriteString(html_writer.str());
            html_writer.clear();
            m_attributesReadPosition = m_file.FlushAndGetPosition();
            m_file.WriteFormattedString(_T("%*s"), 43, L"");

            html_writer << L"</td><td>";
            m_file.WriteString(html_writer.str());
            html_writer.clear();
            m_attributesIgnoredPosition = m_file.FlushAndGetPosition();
            m_file.WriteFormattedString(_T("%*s"), 20, L"");

            html_writer << L"</td><td>";
            m_file.WriteString(html_writer.str());
            html_writer.clear();
            m_attributesUnknownPosition = m_file.FlushAndGetPosition();
            m_file.WriteFormattedString(_T("%*s"), 20, L"");

            html_writer << L"</td><td>";
            m_file.WriteString(html_writer.str());
            html_writer.clear();
            m_attributesErasedPosition = m_file.FlushAndGetPosition();
            m_file.WriteFormattedString(_T("%*s"), 20, L"");
            html_writer << L"</td></tr>\n";

            if (m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records)
            {
                // write out HTML for level summaries
                html_writer << L"<tfooter>\n<tr class='highlight'><td>Level</td><td>Input Case</td>"
                    L"<td>Bad Struct</td><td>Level Post</td></tr>\n";

                for (size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number)
                {
                    LevelSummaryPosition levelSummaryPosition;

                    html_writer << L"<tr><td>";
                    m_file.WriteString(html_writer.str());
                    html_writer.clear();
                    levelSummaryPosition.levelNumberPosition = m_file.FlushAndGetPosition();
                    m_file.WriteFormattedString(_T("%*s"), 20, L"");

                    html_writer << L"</td><td>";
                    m_file.WriteString(html_writer.str());
                    html_writer.clear();

                    levelSummaryPosition.inputCasesPosition = m_file.FlushAndGetPosition();
                    m_file.WriteFormattedString(_T("%*s"), 20, L"");

                    html_writer << L"</td><td>";
                    m_file.WriteString(html_writer.str());
                    html_writer.clear();
                    levelSummaryPosition.badStructuresPosition = m_file.FlushAndGetPosition();
                    m_file.WriteFormattedString(_T("%*s"), 20, L"");

                    html_writer << L"</td><td>";
                    m_file.WriteString(html_writer.str());
                    html_writer.clear();
                    levelSummaryPosition.levelPostsPosition = m_file.FlushAndGetPosition();
                    m_file.WriteFormattedString(_T("%*s"), 20, L"");

                    html_writer << L"</td></tr>\n";
                    m_levelSummaryPositions.push_back(levelSummaryPosition);
                }

                // write out HTML for messages
                html_writer << L"<tr class='highlight'></td><td>Total Messages</td></td><td class='error'>Error</td><td class='warning'>Warning</td><td class='user-defined'>User Defined</td></tr>\n";

                html_writer << L"<tr></td><td>";
                m_file.WriteString(html_writer.str());
                html_writer.clear();
                m_totalMessagePosition = m_file.FlushAndGetPosition();
                m_file.WriteFormattedString(_T("%*s"), 20, L"");

                html_writer << L"</td><td>";
                m_file.WriteString(html_writer.str());
                html_writer.clear();
                m_errorMessagesPosition = m_file.FlushAndGetPosition();
                m_file.WriteFormattedString(_T("%*s"), 20, L"");

                html_writer << L"</td><td>";
                m_file.WriteString(html_writer.str());
                html_writer.clear();
                m_warningMessagesPosition = m_file.FlushAndGetPosition();
                m_file.WriteFormattedString(_T("%*s"), 20, L"");

                html_writer << L"</td><td>";
                m_file.WriteString(html_writer.str());
                html_writer.clear();
                m_userMessagesPosition = m_file.FlushAndGetPosition();
                m_file.WriteFormattedString(_T("%*s"), 20, L"");

                html_writer << L"</td></tr>\n</tbody>\n";
            }

            html_writer << L"</tfooter>\n</table>\n<br>\n";
            m_file.WriteString(html_writer.str());
        }

        void WriteValues()
        {
            // write out values for the number of records (or slices) read
            ASSERT(m_attributesReadPosition.has_value());
            m_file.Seek(*m_attributesReadPosition, SEEK_SET);
            std::wstring recordsRead = FormatTextCS2WS(_T("%d (%d%%)"), m_processSummary->GetAttributesRead(), m_processSummary->GetPercentSourceRead());
            m_file.WriteFormattedString(_T("%*s"), 43, recordsRead.c_str());

            // write out values for bad records (or slices)
            ASSERT(m_attributesIgnoredPosition.has_value());
            m_file.Seek(*m_attributesIgnoredPosition, SEEK_SET);
            m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetAttributesIgnored());

            ASSERT(m_attributesUnknownPosition.has_value());
            m_file.Seek(*m_attributesUnknownPosition, SEEK_SET);
            m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetAttributesUnknown());

            ASSERT(m_attributesErasedPosition.has_value());
            m_file.Seek(*m_attributesErasedPosition, SEEK_SET);
            m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetAttributesErased());

            if (m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records)
            {
                // write out values for messages
                ASSERT(m_totalMessagePosition.has_value());
                m_file.Seek(*m_totalMessagePosition, SEEK_SET);
                m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetTotalMessages());

                ASSERT(m_errorMessagesPosition.has_value());
                m_file.Seek(*m_errorMessagesPosition, SEEK_SET);
                m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetErrorMessages());

                ASSERT(m_warningMessagesPosition.has_value());
                m_file.Seek(*m_warningMessagesPosition, SEEK_SET);
                m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetWarningMessages());

                ASSERT(m_userMessagesPosition.has_value());
                m_file.Seek(*m_userMessagesPosition, SEEK_SET);
                m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetUserMessages());

                ASSERT(m_levelSummaryPositions.size() == m_processSummary->GetNumberLevels());
                
                for (size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number)
                {
                    // write out the level summaries
                    const LevelSummaryPosition& levelSummaryPosition = m_levelSummaryPositions[level_number];

                    ASSERT(levelSummaryPosition.levelNumberPosition.has_value());
                    m_file.Seek(*levelSummaryPosition.levelNumberPosition, SEEK_SET);
                    m_file.WriteFormattedString(_T("%*zu"), 20, level_number + 1);

                    ASSERT(levelSummaryPosition.inputCasesPosition.has_value());
                    m_file.Seek(*levelSummaryPosition.inputCasesPosition, SEEK_SET);
                    m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetCaseLevelsRead(level_number));

                    ASSERT(levelSummaryPosition.badStructuresPosition.has_value());
                    m_file.Seek(*levelSummaryPosition.badStructuresPosition, SEEK_SET);
                    m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetBadCaseLevelStructures(level_number));

                    ASSERT(levelSummaryPosition.levelPostsPosition.has_value());
                    m_file.Seek(*levelSummaryPosition.levelPostsPosition, SEEK_SET);
                    m_file.WriteFormattedString(_T("%*zu"), 20, m_processSummary->GetLevelPostProcsExecuted(level_number));
                }
            }

            m_file.SeekToEnd();
        }

    private:
        std::shared_ptr<ProcessSummary> m_processSummary;
        CStdioFileUnicode& m_file;
        std::optional<ULONGLONG> m_processSummaryPosition;

        std::optional<ULONGLONG> m_attributesReadPosition;
        std::optional<ULONGLONG> m_attributesTypePosition;

        std::optional<ULONGLONG> m_attributesIgnoredPosition;
        std::optional<ULONGLONG> m_attributesUnknownPosition;
        std::optional<ULONGLONG> m_attributesErasedPosition;

        std::optional<ULONGLONG> m_totalMessagePosition;
        std::optional<ULONGLONG> m_errorMessagesPosition;
        std::optional<ULONGLONG> m_warningMessagesPosition;
        std::optional<ULONGLONG> m_userMessagesPosition;

        std::vector<LevelSummaryPosition> m_levelSummaryPositions;
    };
}



Listing::HtmlLister::HtmlLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, const PFF& pff)
    :   Lister(std::move(process_summary)),
        m_writeProcessSummaryAndMessages(pff.GetAppType() != APPTYPE::ENTRY_TYPE),
        m_isProcessMessageComplete(false),
        m_isMultiLevel(false)
{
    m_hasData = ( PortableFunctions::FileSize(filename) > Utf8BOM_sv.length() );

    m_file = OpenListingFile(filename, append);
}


Listing::HtmlLister::~HtmlLister()
{
    if (m_file != nullptr)
        m_file->Close();
}


void Listing::HtmlLister::WriteHeader(const std::vector<HeaderAttribute>& header_attributes)
{
    HtmlStringWriter html_writer;

    if (m_hasData)
    {
        MoveToHtmlEndTag();
    }
    else
    {
        html_writer.WriteDefaultHeader(L"HTML Listing", Html::CSS::Common);
        html_writer << L"\n<body class='container-page'>\n";
    }

    html_writer << L"<table>\n";
    html_writer << L"<thead>\n<tr><th colspan='2' class='center'>Listing Details</th></tr>\n</thead>\n";

    // write out the header attributes
    for (const HeaderAttribute& header_attribute : header_attributes)
    {
        html_writer << L"<tr>";

        html_writer << L"<td class='left width-10'>" << header_attribute.description << L"</td>";

        std::wstring secondary_description;
        if (header_attribute.secondary_description.has_value())
        {
            secondary_description = *header_attribute.secondary_description + L"=";
        }

        if (std::holds_alternative<std::wstring>(header_attribute.value))
        {
            html_writer << L"<td class='left'>" << secondary_description << std::get<std::wstring>(header_attribute.value) << L"</td>";
        }
        else
        {
            const ConnectionString& connection_string = std::get<ConnectionString>(header_attribute.value);
            std::optional<std::wstring> data_uri = CreateDataUri(connection_string, *header_attribute.dictionary);

            html_writer << L"<td class='left'>" << secondary_description;

            if( data_uri.has_value() )
                html_writer << L"<a href=\"" << data_uri->c_str() << L"\">";

            html_writer << connection_string.GetName(DataRepositoryNameType::ForListing);

            if( data_uri.has_value() )
                html_writer << L"</a>";

            html_writer << L"</td>";
        }

        html_writer << L"</tr>\n";
    }

    // write out the date
    html_writer << L"<tr><td class='left'>Date</td><td class='left'>" << GetSystemDate() << L"</td></tr>\n";

    std::wstring time = GetSystemTime();
    html_writer << L"<tr><td class='left'>Start Time</td><td class='left'>" << time << L"</td></tr>\n";
    html_writer << L"<tr><td class='left'>End Time</td><td class='left'>" << time;
    m_file->WriteString(html_writer.str());
    html_writer.clear();
    m_endTimePosition = m_file->FlushAndGetPosition() - time.length();

    html_writer << L"</td></tr>\n</table>\n<br>\n";
    m_file->WriteString(html_writer.str());

    if (!m_writeProcessSummaryAndMessages)
        return;

    m_file->WriteString(html_writer.str());
    html_writer.clear();

    m_processSummaryFormatter = std::make_unique<ProcessSummaryHTMLFormatter>(m_processSummary, *m_file);
    m_processSummaryFormatter->WriteShell();

    m_startProcessMessagePosition = m_file->FlushAndGetPosition();
    m_file->WriteFormattedString(_T("%*s\n"), 110, L"");
}


void Listing::HtmlLister::WriteMessages(const Messages& messages)
{
    static constexpr const TCHAR* TypeCodes[] =
    {
        _T("A"),
        _T("E"),
        _T("W"),
        _T("U")
    };

    HtmlStringWriter html_writer;
    // WriteProcessMessageTableTags() will write table tags

    // write the message source
    html_writer << L"<tr class='highlight'>";

    const TCHAR* colspan_size = m_isMultiLevel ? L"3" : L"2";

    if (m_caseKeyUuid.has_value())
    {
        // construct the full link to open the case in Data Viewer 
        std::optional<std::wstring> case_uri = CreateCaseUri(m_inputDataUri, m_caseKeyUuid);

        html_writer << L"<td colspan='" << colspan_size << L"' class='left'>Case [";

        if( case_uri.has_value() )
            html_writer << L"<a href=\"" << case_uri->c_str() << L"\">";

        html_writer << messages.source;

        if( case_uri.has_value() )
            html_writer << L"</a>";

        html_writer << L"]</td>";
    }
    else
    {
        html_writer << L"<td colspan='" << colspan_size << L"' class='left'>" << messages.source << L"</td>";
    }

    html_writer << L"</tr>\n";

    // write the messages
    for (const Message& message : messages.messages)
    {
        // write a message
        if (message.details.has_value())
        {
            ASSERT((size_t)message.details->type < _countof(TypeCodes));
            const TCHAR* type_code = TypeCodes[(size_t)message.details->type];

            const TCHAR* message_style = L"";
            if (message.details->type == MessageType::Error)
                message_style = L"error";
            else if (message.details->type == MessageType::Warning)
                message_style = L"warning";
            else if (message.details->type == MessageType::User)
                message_style = L"user-defined";

            html_writer << L"<tr>";

            if (m_isMultiLevel)
            {
                std::wstring level_key;
                if (!message.level_key.empty())
                    level_key = L"[" + message.level_key + L"]";

                html_writer << L"<td class='left width-10'>" << level_key << L"</td>";
            }

            html_writer << L"<td class='" << message_style << L" left width-10'>" << type_code << L"" << IntToString(message.details->number) << L"</td><td class='left'>" << message.text << L"</td></tr>\n";
        }
        else
        {
            // there is no special formatting for text coming from the write function
            html_writer << L"<tr><td class='left'>" << message.text << L"</td></tr>\n";
        }
    }

    m_file->WriteString(html_writer.str());
}


void Listing::HtmlLister::ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    m_inputDataUri = CreateDataUri(connection_string, dictionary);
    m_isMultiLevel = ( dictionary.GetNumLevels() > 1 );
}


void Listing::HtmlLister::ProcessCaseSource(const Case* data_case)
{
    if( data_case == nullptr )
        m_caseKeyUuid.reset();

    else
        m_caseKeyUuid.emplace(CS2WS(data_case->GetKey()), CS2WS(data_case->GetUuid()));
}


void Listing::HtmlLister::WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries)
{
    if (m_writeProcessSummaryAndMessages)
        WriteUpdatesToProcessMessageTable();

    ASSERT(!message_summaries.empty());

    MessageSummary::Type message_summaries_type = message_summaries.front().type;
    const TCHAR* type_text = (message_summaries_type == MessageSummary::Type::System) ? _T("System") :
        (message_summaries_type == MessageSummary::Type::UserNumbered) ? _T("User Numbered") :
        _T("User Unnumbered");

    HtmlStringWriter html_writer;

    // write the header
    html_writer << L"<br><br>\n<table>\n";

    int column_count = 5;
    if (message_summaries_type == MessageSummary::Type::System)
        column_count = 3;

    html_writer << L"<thead>\n<tr><th colspan='" << IntToString(column_count) << L"' class='center'>" << type_text << L" Messages</th></tr>\n</thead>\n";

    bool use_denoms = (message_summaries_type != MessageSummary::Type::System);
    const std::wstring number_heading = (message_summaries_type == MessageSummary::Type::UserUnnumbered) ? _T("Line") : _T("Number");
    const std::wstring frequency_heading = _T("Freq");
    const std::wstring percent_heading = _T("%");
    const std::wstring message_heading = _T("Message Text");
    const std::wstring denom_heading = _T("Denom");

    html_writer << L"<tr class='highlight'><td class='width-10'>" << number_heading << L"</td><td class='width-10'>" << frequency_heading << L"</td>";

    if (use_denoms)
    {
        html_writer << L"<td class='width-10'>" << percent_heading << L"</td>";
    }

    html_writer << L"<td class='left'>" << message_heading << L"</td>";

    if (use_denoms)
    {
        html_writer << L"<td class='width-10'>" << denom_heading << L"</td>";
    }

    html_writer << L"</tr>\n";

    // write the messages
    const std::wstring blank_percent_denom_text = _T("-");
    std::wstring percent_text;
    std::wstring denom_text;

    for (const MessageSummary& message_summary : message_summaries)
    {
        bool has_a_denom = (use_denoms && message_summary.denominator.has_value());
        bool has_a_blank_denom = (use_denoms && !message_summary.denominator.has_value());

        if (has_a_denom)
        {
            bool calculate_percent = false;

            if (*message_summary.denominator < 0 || IsSpecial(*message_summary.denominator))
            {
                denom_text = SO::GetRepeatingCharacterString('*', denom_heading.length());
            }
            else
            {
                denom_text = IntToString((uint64_t)*message_summary.denominator);

                if (*message_summary.denominator != 0 && message_summary.frequency <= *message_summary.denominator)
                    calculate_percent = true;
            }

            if (calculate_percent)
            {
                percent_text = FormatTextCS2WS(_T("%5.1f"), CreatePercent<double>(message_summary.frequency, *message_summary.denominator));
            }
            else
            {
                percent_text = SO::GetRepeatingCharacterString('*', percent_heading.length());
            }
        }

        // show unnumbered messages' line numbers as positive (rather than negative as they appear elsewhere)
        int message_number_for_display = std::abs(message_summary.message_number);

        html_writer << L"<tr><td>" << IntToString(message_number_for_display) << L"</td><td>" << IntToString(message_summary.frequency) << L"</td>";

        if (has_a_denom)
        {
            html_writer << L"<td>" << percent_text << L"</td>";
        }
        else if (has_a_blank_denom)
        {
            html_writer << L"<td>" << blank_percent_denom_text << L"</td>";
        }

        html_writer << L"<td class='left'>" << message_summary.message_text << L"</td>";

        if (has_a_denom)
        {
            html_writer << L"<td>" << denom_text << L"</td>";
        }
        else if (has_a_blank_denom)
        {
            html_writer << L"<td>" << blank_percent_denom_text << L"</td>";
        }
        html_writer << L"</tr>\n";
    }

    html_writer << L"</table>\n";
    m_file->WriteString(html_writer.str());
}


void Listing::HtmlLister::WriteWarningAboutApplicationErrors(const std::wstring& application_errors_filename)
{
    HtmlStringWriter html_writer;
    html_writer << L"<br>\n<div class='center'><em>A compilation error file "
                << L"<a href=\"" << CreateTextViewerUri(application_errors_filename).c_str() << L"\">"
                << L"(" << application_errors_filename << L")</a> has been created for your application."
                << L"</em></div>\n";
    m_file->WriteString(html_writer.str());
}


void Listing::HtmlLister::UpdateProcessSummary()
{
    if (m_processSummaryFormatter != nullptr)
        m_processSummaryFormatter->WriteValues();
}


void Listing::HtmlLister::WriteFooter()
{
    HtmlStringWriter html_writer;
    html_writer << L"<br>\n<div class='center'>CSPro Executor Normal End</div>\n<br>\n";
    html_writer << L"<hr>\n";
    html_writer << L"</body>\n</html>\n";
    m_file->WriteString(html_writer.str());

    // update the end time
    if (m_endTimePosition.has_value())
    {
        m_file->Seek(*m_endTimePosition, SEEK_SET);
        m_file->WriteString(GetSystemTime().c_str());
        m_file->SeekToEnd();
    }
}


void Listing::HtmlLister::WriteUpdatesToProcessMessageTable()
{
    if (!m_isProcessMessageComplete)
    {
        m_isProcessMessageComplete = true;

        const TCHAR* colspan_size = m_isMultiLevel ? L"3" : L"2";

        m_file->Seek(*m_startProcessMessagePosition, SEEK_SET);
        m_file->WriteFormattedString(_T("<br><table><thead><tr><th colspan='%s' class='center'>Process Messages</td></tr></thead>"), colspan_size);

        m_file->SeekToEnd();
        m_file->WriteString(L"</table>\n");
    }
}


void Listing::HtmlLister::MoveToHtmlEndTag() const
{
    // use length of L"</body>\n</html>\n" to calculate offset
    const LONGLONG offset = -18;
    m_file->Seek(offset, SEEK_END);
}
