#include "stdafx.h"
#include "TextLister.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/Special.h>


namespace
{
    constexpr size_t MaximumRecordWidth = 10;
    constexpr size_t MaximumSmallerNumberWidth = 7;
    constexpr size_t MaximumMessageNumberWidth = 8;

    constexpr size_t TableMarginWidth = 6;
    constexpr size_t CellPaddingWidth = 2;
    constexpr size_t MaximumLevelWidth = 1;
    constexpr const TCHAR* LevelText = _T("Level");
    constexpr const TCHAR* InputCaseText = _T("Input Case");
    constexpr const TCHAR* BadStructText = _T("Bad Struct");
    constexpr const TCHAR* LevelPostText = _T("Level Post");
    constexpr TCHAR MarginChar = _T(' ');
    constexpr TCHAR TopLeftChar = _T('╔');
    constexpr TCHAR TopRightChar = _T('╗');
    constexpr TCHAR BottomLeftChar = _T('╚');
    constexpr TCHAR BottomRightChar = _T('╝');
    constexpr TCHAR HorizontalChar = _T('═');
    constexpr TCHAR VerticalChar = _T('║');
    constexpr TCHAR TopCellSeparatorChar = _T('╦');
    constexpr TCHAR MiddleCellSeparatorChar = _T('╬');
    constexpr TCHAR BottomCellSeparatorChar = _T('╩');
    constexpr TCHAR LeftVerticalCellSeparatorChar = _T('╠');
    constexpr TCHAR RightVerticalCellSeparatorChar = _T('╣');
}


// --------------------------------------------------
// ProcessSummaryFormatter
// --------------------------------------------------

namespace Listing
{
    class ProcessSummaryFormatter
    {
    public:
        ProcessSummaryFormatter(std::shared_ptr<ProcessSummary> process_summary, CStdioFileUnicode& file);
        void WriteShell();
        void WriteValues();

    private:
        void WriteLine(const TCHAR* text);
        void WriteLine(const std::wstring& text) { WriteLine(text.c_str()); }

        std::wstring CreateLine(TCHAR left_char, TCHAR middle_char, TCHAR right_char, TCHAR cell_char = 0);

    private:
        std::shared_ptr<ProcessSummary> m_processSummary;
        CStdioFileUnicode& m_file;
        std::optional<ULONGLONG> m_processSummaryPosition;
        std::wstring m_marginText;
        std::vector<size_t> m_cellWidths;
        size_t m_tableWidth;
        std::wstring m_cellsTopBorderLine;
        std::wstring m_cellsFormatter;
        std::wstring m_cellsMiddleBorderLine;
    };
}


Listing::ProcessSummaryFormatter::ProcessSummaryFormatter(std::shared_ptr<ProcessSummary> process_summary, CStdioFileUnicode& file)
    :   m_processSummary(std::move(process_summary)),
        m_file(file),
        m_marginText(TableMarginWidth, MarginChar),
        m_tableWidth(0)
{
    auto add_cell = [&](const TCHAR* text, size_t maximum_value_width)
    {
        size_t cell_width = std::max(_tcslen(text), maximum_value_width);
        m_cellWidths.emplace_back(cell_width);
        m_tableWidth += cell_width;
    };

    add_cell(LevelText, MaximumLevelWidth);
    add_cell(InputCaseText, MaximumRecordWidth);
    add_cell(BadStructText, MaximumRecordWidth);
    add_cell(LevelPostText, MaximumRecordWidth);

    // add the borders and padding to the table size
    m_tableWidth += 1 + m_cellWidths.size() * ( 2 * CellPaddingWidth + 1 );
}


void Listing::ProcessSummaryFormatter::WriteShell()
{
    WriteLine(CreateLine(TopLeftChar, HorizontalChar, TopRightChar));
    m_processSummaryPosition = m_file.FlushAndGetPosition();

    const std::wstring non_cell_line = CreateLine(VerticalChar, ' ', VerticalChar);
    WriteLine(non_cell_line);
    WriteLine(non_cell_line);

    if( m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        ASSERT(m_processSummary->GetNumberLevels() > 0);

        WriteLine(non_cell_line);

        m_cellsTopBorderLine = WS2CS(CreateLine(LeftVerticalCellSeparatorChar, HorizontalChar, RightVerticalCellSeparatorChar, TopCellSeparatorChar));
        WriteLine(m_cellsTopBorderLine);

        // create the cell formatter
        for( size_t cell_width : m_cellWidths )
        {
            SO::AppendFormat(m_cellsFormatter, _T("%c%*s%%%ds%*s"), VerticalChar, static_cast<int>(CellPaddingWidth), _T(""),
                                               static_cast<int>(cell_width), static_cast<int>(CellPaddingWidth), _T(""));
        }

        m_cellsFormatter.push_back(VerticalChar);

        const std::wstring empty_cell_text = FormatTextCS2WS(m_cellsFormatter.c_str(), _T(""), _T(""), _T(""), _T(""));
        WriteLine(empty_cell_text);

        m_cellsMiddleBorderLine = WS2CS(CreateLine(LeftVerticalCellSeparatorChar, HorizontalChar, RightVerticalCellSeparatorChar, MiddleCellSeparatorChar));
        WriteLine(m_cellsMiddleBorderLine);

        for( size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number )
            WriteLine(empty_cell_text);

        WriteLine(CreateLine(BottomLeftChar, HorizontalChar, BottomRightChar, BottomCellSeparatorChar));
    }

    else
    {
        WriteLine(CreateLine(BottomLeftChar, HorizontalChar, BottomRightChar));
    }

    m_file.WriteLine();
}


void Listing::ProcessSummaryFormatter::WriteValues()
{
    ASSERT(m_processSummaryPosition.has_value());
    m_file.Seek(*m_processSummaryPosition, SEEK_SET);

    auto write_summary_line = [&](std::wstring summary_line)
    {
        SO::MakeExactLength(summary_line, std::max(m_tableWidth, summary_line.length() + 1));
        summary_line.back() = VerticalChar;
        WriteLine(summary_line);
    };

    // write out information on the number of records (or slices) read
    write_summary_line(FormatTextCS2WS(_T("%c%*d %s Read   %d%% of input file"), VerticalChar,
                                       static_cast<int>(MaximumRecordWidth), static_cast<int>(m_processSummary->GetAttributesRead()),
                                       ( m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records ) ? _T("Records") : _T("Slices"),
                                       static_cast<int>(m_processSummary->GetPercentSourceRead())));

    // write out information on bad records (or slices)
    write_summary_line(FormatTextCS2WS(_T("%c%*d Ignored: %*d Unknown   %*d Erased"), VerticalChar,
                                       static_cast<int>(MaximumRecordWidth),        static_cast<int>(m_processSummary->GetAttributesIgnored()),
                                       static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary->GetAttributesUnknown()),
                                       static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary->GetAttributesErased())));

    if( m_processSummary->GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        // write out information on messages
        write_summary_line(FormatTextCS2WS(_T("%c%*d Messages:%*d E%*d W%*d User"), VerticalChar,
                                           static_cast<int>(MaximumRecordWidth),        static_cast<int>(m_processSummary->GetTotalMessages()),
                                           static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary->GetErrorMessages()),
                                           static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary->GetWarningMessages()),
                                           static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary->GetUserMessages())));

        // write the level summaries
        WriteLine(m_cellsTopBorderLine);

        WriteLine(FormatTextCS2WS(m_cellsFormatter.c_str(), LevelText, InputCaseText, BadStructText, LevelPostText));

        WriteLine(m_cellsMiddleBorderLine);

        for( size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number )
        {
            WriteLine(FormatText(m_cellsFormatter.c_str(), IntToString(level_number + 1).GetString(),
                                                           IntToString(m_processSummary->GetCaseLevelsRead(level_number)).GetString(),
                                                           IntToString(m_processSummary->GetBadCaseLevelStructures(level_number)).GetString(),
                                                           IntToString(m_processSummary->GetLevelPostProcsExecuted(level_number)).GetString()));
        }
    }

    m_file.SeekToEnd();
}


void Listing::ProcessSummaryFormatter::WriteLine(const TCHAR* text)
{
    ASSERT80(_tcslen(text) == m_tableWidth);

    m_file.WriteString(m_marginText);
    m_file.WriteLine(text);
}


std::wstring Listing::ProcessSummaryFormatter::CreateLine(TCHAR left_char, TCHAR middle_char, TCHAR right_char, TCHAR cell_char/* = 0*/)
{
    std::wstring line = SO::GetRepeatingCharacterString(middle_char, m_tableWidth);
    line.front() = left_char;
    line.back() = right_char;

    if( cell_char != 0 )
    {
        size_t cell_border_position = 0;

        for( size_t i = 1; i < m_cellWidths.size(); ++i )
        {
            cell_border_position += 1 + m_cellWidths[i - 1] + 2 * CellPaddingWidth;
            line[cell_border_position] = cell_char;
        }
    }

    return line;
}



// --------------------------------------------------
// TextLister
// --------------------------------------------------

Listing::TextLister::TextLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, const PFF& pff)
    :   Lister(std::move(process_summary)),
        m_listingWidth(pff.GetListingWidth()),
        m_wrapMessages(pff.GetMessageWrap()),
        m_messagesAreFromCase(false),
        m_writeProcessSummaryAndMessages(pff.GetAppType() != APPTYPE::ENTRY_TYPE)
{
    m_file = OpenListingFile(filename, append);
}


Listing::TextLister::~TextLister()
{
    if( m_file != nullptr )
    {
        m_file->Close();
        m_file.reset();
    }
}


void Listing::TextLister::WriteHeader(const std::vector<HeaderAttribute>& header_attributes)
{
#define DescriptionFormatter _T("%-15s")

    // write out the header attributes
    for( const HeaderAttribute& header_attribute : header_attributes )
    {
        m_file->WriteFormattedString(DescriptionFormatter _T(" "), header_attribute.description.c_str());

        if( header_attribute.secondary_description.has_value() )
            m_file->WriteFormattedString(_T("%s="), header_attribute.secondary_description->c_str());

        if( std::holds_alternative<std::wstring>(header_attribute.value) )
        {
            m_file->WriteLine(std::get<std::wstring>(header_attribute.value));
        }

        else
        {
            m_file->WriteLine(std::get<ConnectionString>(header_attribute.value).GetName(DataRepositoryNameType::ForListing));
        }
    }

    // write out the date
    m_file->WriteFormattedLine(_T("\n") DescriptionFormatter _T(" %s"), _T("Date"), GetSystemDate().c_str());

    const std::wstring time = GetSystemTime();
    m_file->WriteFormattedLine(DescriptionFormatter _T(" %s"), _T("Start Time"), time.c_str());
    m_file->WriteFormattedString(DescriptionFormatter _T(" %s"), _T("End Time"), time.c_str());
    m_endTimePosition = m_file->FlushAndGetPosition() - time.length();

    m_file->WriteLine(_T("\n"));

    if( !m_writeProcessSummaryAndMessages )
        return;

    m_file->WriteLine(_T("CSPro Process Summary\n"));

    m_processSummaryFormatter = std::make_unique<ProcessSummaryFormatter>(m_processSummary, *m_file);
    m_processSummaryFormatter->WriteShell();

    m_file->WriteLine(_T("Process Messages"));
}


void Listing::TextLister::WriteMessages(const Messages& messages)
{
    static constexpr const TCHAR* TypeCodes[] =
    {
        _T("A"),
        _T("E"),
        _T("W"),
        _T("U")
    };

    // write the message source
    m_file->WriteString(_T("\n*** "));

    if( m_messagesAreFromCase )
    {
        // make sure that all messages have the same level key
        const std::wstring& level_key = messages.messages.front().level_key;
        ASSERT(static_cast<size_t>(std::count_if(messages.messages.cbegin(), messages.messages.cend(),
                                                 [&](const Message& message) { return ( message.level_key == level_key ); })) == messages.messages.size());

        auto write_key_in_brackets = [&](const std::wstring& key_text)
        {
            m_file->WriteString(_T("["));

            // when newlines are present in the key, replace them with ␤
            if( SO::ContainsNewlineCharacter(key_text) )
            {
                m_file->WriteString(NewlineSubstitutor::NewlineToUnicodeNL(key_text));
            }

            else
            {
                m_file->WriteString(key_text);
            }

            m_file->WriteString(_T("]"));
        };

        m_file->WriteString(_T("Case "));

        write_key_in_brackets(messages.source);

        if( !level_key.empty() )
            write_key_in_brackets(level_key);
    }

    else
    {
        ASSERT(!SO::ContainsNewlineCharacter(messages.source));

        m_file->WriteString(messages.source);
    }

    // write the number of messages
    auto [num_errors, num_warnings, num_user_messages, num_total_messages] = CountMessages();

    m_file->WriteFormattedLine(_T(" has %d message%s (%d E / %d W / %d U)"),
                               static_cast<int>(num_total_messages), PluralizeWord(num_total_messages),
                               static_cast<int>(num_errors), static_cast<int>(num_warnings), static_cast<int>(num_user_messages));

    constexpr size_t MessageNumberPadding = 14; // the padding, the type, and the number
    const size_t maximum_message_width = m_listingWidth - MessageNumberPadding;

    // write the messages
    for( const Message& message : messages.messages )
    {
        const TCHAR* type_code;

        // write a message
        if( message.details.has_value() )
        {
            ASSERT(static_cast<size_t>(message.details->type) < _countof(TypeCodes));
            type_code = TypeCodes[static_cast<size_t>(message.details->type)];

            m_file->WriteFormattedString(_T("    %s %7d "), type_code, message.details->number);

            // write the entire message or, if necessary, wrap it on multiple lines
            if( message.text.length() <= maximum_message_width && !SO::ContainsNewlineCharacter(message.text) )
            {
                m_file->WriteLine(message.text);
            }

            else
            {
                bool add_padding = false;

                for( const std::wstring& line : SO::WrapText(message.text, maximum_message_width) )
                {
                    ASSERT(!SO::ContainsNewlineCharacter(line));

                    if( add_padding )
                    {
                        m_file->WriteFormattedString(_T("%*s"), static_cast<int>(MessageNumberPadding), _T(""));
                    }

                    else
                    {
                        add_padding = true;
                    }

                    m_file->WriteLine(line);
                }
            }
        }

        // there is no special formatting for text coming from the write function
        else
        {
            m_file->WriteLine(message.text);
        }
    }
}


void Listing::TextLister::ProcessCaseSource(const Case* data_case)
{
    m_messagesAreFromCase = ( data_case != nullptr );
}


void Listing::TextLister::WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries)
{
    ASSERT(!message_summaries.empty());

    const MessageSummary::Type& message_summaries_type = message_summaries.front().type;
    const TCHAR* const type_text = ( message_summaries_type == MessageSummary::Type::System )       ? _T("System") :
                                   ( message_summaries_type == MessageSummary::Type::UserNumbered ) ? _T("User numbered") :
                                                                                                      _T("User unnumbered");

    // write the header
    m_file->WriteFormattedLine(_T("\n%s messages:\n"), type_text);

    // calculate the width of each column
    constexpr size_t ColumnPadding = 2;
    constexpr size_t MaximumPercentWidth = 5;

    const size_t maximum_message_text_width = m_listingWidth - MaximumMessageNumberWidth - MaximumRecordWidth -
                                              MaximumPercentWidth - MaximumRecordWidth - ( 4 * ColumnPadding );
    ASSERT(maximum_message_text_width > 0 && maximum_message_text_width < m_listingWidth);

    auto write_formatted_line = [&](const TCHAR* number, const TCHAR* frequency, const TCHAR* percent, const TCHAR* text, const TCHAR* denom)
    {
        ASSERT(!SO::ContainsNewlineCharacter(wstring_view(text)));

        m_file->WriteFormattedLine(_T("%*s  %*s  %*s  %-*s  %*s"),
                                   static_cast<int>(MaximumMessageNumberWidth), number,
                                   static_cast<int>(MaximumRecordWidth), frequency,
                                   static_cast<int>(MaximumPercentWidth), percent,
                                   static_cast<int>(maximum_message_text_width), text,
                                   static_cast<int>(MaximumRecordWidth), denom);
    };

    const bool use_denoms = ( message_summaries_type != MessageSummary::Type::System );
    const std::wstring number_heading = ( message_summaries_type == MessageSummary::Type::UserUnnumbered ) ? _T("Line") : _T("Number");
    const std::wstring frequency_heading = _T("Freq");
    const std::wstring percent_heading = use_denoms ? _T("  %  ") : _T("");
    const std::wstring message_heading = _T("Message Text");
    const std::wstring denom_heading = use_denoms ? _T("Denom") : _T("");

    write_formatted_line(number_heading.c_str(), frequency_heading.c_str(), percent_heading.c_str(), message_heading.c_str(), denom_heading.c_str());

    write_formatted_line(SO::GetDashedLine(number_heading.length()),
                         SO::GetDashedLine(frequency_heading.length()),
                         SO::GetDashedLine(percent_heading.length()),
                         SO::GetDashedLine(message_heading.length()),
                         SO::GetDashedLine(denom_heading.length()));

    // write the messages
    const std::wstring blank_percent_denom_text = use_denoms ? _T("-") : _T("");
    std::wstring percent_text;
    std::wstring denom_text;
    std::vector<std::wstring> wrapped_lines;

    for( const MessageSummary& message_summary : message_summaries )
    {
        const std::wstring* first_line = &message_summary.message_text;

        if( first_line->size() > maximum_message_text_width || SO::ContainsNewlineCharacter(*first_line) )
        {
            wrapped_lines = SO::WrapText(*first_line, maximum_message_text_width);
            first_line = &wrapped_lines.front();
        }

        else
        {
            wrapped_lines.clear();
        }

        const bool has_a_denom = ( use_denoms && message_summary.denominator.has_value() );

        if( has_a_denom )
        {
            bool calculate_percent = false;

            if( *message_summary.denominator < 0 || IsSpecial(*message_summary.denominator) )
            {
                denom_text = SO::GetRepeatingCharacterString('*', denom_heading.length());
            }

            else
            {
                denom_text = IntToString(static_cast<uint64_t>(*message_summary.denominator));

                if( *message_summary.denominator != 0 && message_summary.frequency <= *message_summary.denominator )
                    calculate_percent = true;
            }

            if( calculate_percent )
            {
                percent_text = FormatTextCS2WS(_T("%5.1f"), CreatePercent<double>(message_summary.frequency, *message_summary.denominator));
            }

            else
            {
                percent_text = SO::GetRepeatingCharacterString('*', percent_heading.length());
            }
        }

        // show unnumbered messages' line numbers as positive (rather than negative as they appear elsewhere)
        const int message_number_for_display = std::abs(message_summary.message_number);

        // write the first line
        write_formatted_line(IntToString(message_number_for_display).GetString(),
                             IntToString(message_summary.frequency).GetString(),
                             has_a_denom ? percent_text.c_str() : blank_percent_denom_text.c_str(),
                             first_line->c_str(),
                             has_a_denom ? denom_text.c_str() : blank_percent_denom_text.c_str());

        // write any wrapped lines
        for( size_t i = 1; i < wrapped_lines.size(); ++i )
            write_formatted_line(_T(""), _T(""), _T(""), wrapped_lines[i].c_str(), _T(""));
    }
}


void Listing::TextLister::WriteWarningAboutApplicationErrors(const std::wstring& application_errors_filename)
{
    m_file->WriteFormattedLine(_T("\n\nA compilation error file (%s) has been created for your application."),
                               PortableFunctions::PathGetFilename(application_errors_filename));
}


void Listing::TextLister::UpdateProcessSummary()
{
    if( m_processSummaryFormatter != nullptr )
        m_processSummaryFormatter->WriteValues();
}


void Listing::TextLister::WriteFooter()
{
    m_file->WriteLine(_T("\n\nCSPro Executor Normal End"));

    // write out a dashed lined (60 and 231 come from work on 20100316)
    m_file->WriteLine(SO::GetDashedLine(std::max<size_t>(60, std::min<size_t>(m_listingWidth, 231))));

    // update the end time
    if( m_endTimePosition.has_value() )
    {
        m_file->Seek(*m_endTimePosition, SEEK_SET);
        m_file->WriteString(GetSystemTime());
        m_file->SeekToEnd();
    }
}


std::optional<std::tuple<bool, Listing::ListingType, void*>> Listing::TextLister::GetFrequencyPrinter()
{
    return std::tuple<bool, Listing::ListingType, void*>(true, ListingType::Text, m_file.get());
}
