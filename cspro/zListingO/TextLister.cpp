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
    constexpr const char* LevelText = "Level";
    constexpr const char* InputCaseText = "Input Case";
    constexpr const char* BadStructText = "Bad Struct";
    constexpr const char* LevelPostText = "Level Post";
    constexpr char MarginChar = ' ';
    constexpr wchar_t TopLeftChar = L'╔';
    constexpr wchar_t TopRightChar = L'╗';
    constexpr wchar_t BottomLeftChar = L'╚';
    constexpr wchar_t BottomRightChar = L'╝';
    constexpr wchar_t HorizontalChar = L'═';
    constexpr wchar_t VerticalChar = L'║';
    constexpr const char* VerticalCharText = u8"║";
    constexpr wchar_t TopCellSeparatorChar = L'╦';
    constexpr wchar_t MiddleCellSeparatorChar = L'╬';
    constexpr wchar_t BottomCellSeparatorChar = L'╩';
    constexpr wchar_t LeftVerticalCellSeparatorChar = L'╠';
    constexpr wchar_t RightVerticalCellSeparatorChar = L'╣';

    constexpr size_t LevelText_length = std::string_view(LevelText).length();
    constexpr size_t InputCaseText_length = std::string_view(InputCaseText).length();
    constexpr size_t BadStructText_length = std::string_view(BadStructText).length();
    constexpr size_t LevelPostText_length = std::string_view(LevelPostText).length();
}


// --------------------------------------------------------------------------
// ProcessSummaryFormatter
// --------------------------------------------------------------------------

class Listing::ProcessSummaryFormatter
{
public:
    ProcessSummaryFormatter(const ProcessSummary& process_summary, FileIO::TextFile& text_file);
    void WriteShell();
    void WriteValues();

private:
    template<typename T>
    void WriteLine(T&& text);

    std::string CreateLine(wchar_t left_char, wchar_t middle_char, wchar_t right_char, wchar_t cell_char = 0);

private:
    const ProcessSummary& m_processSummary;
    FileIO::TextFile& m_textFile;
    std::optional<int64_t> m_processSummaryPosition;
    const std::string m_marginText;
    std::vector<size_t> m_cellWidths;
    size_t m_tableWidth;
    std::string m_cellsTopBorderLine;
    std::string m_cellsFormatter;
    std::string m_cellsMiddleBorderLine;
};



Listing::ProcessSummaryFormatter::ProcessSummaryFormatter(const ProcessSummary& process_summary, FileIO::TextFile& text_file)
    :   m_processSummary(process_summary),
        m_textFile(text_file),
        m_marginText(TableMarginWidth, MarginChar),
        m_tableWidth(0)
{
    auto add_cell = [&](const size_t text_length, const size_t maximum_value_width)
    {
        const size_t cell_width = std::max(text_length, maximum_value_width);
        m_cellWidths.emplace_back(cell_width);
        m_tableWidth += cell_width;
    };

    add_cell(LevelText_length, MaximumLevelWidth);
    add_cell(InputCaseText_length, MaximumRecordWidth);
    add_cell(BadStructText_length, MaximumRecordWidth);
    add_cell(LevelPostText_length, MaximumRecordWidth);

    // add the borders and padding to the table size
    m_tableWidth += 1 + m_cellWidths.size() * ( 2 * CellPaddingWidth + 1 );
}


void Listing::ProcessSummaryFormatter::WriteShell()
{
    WriteLine(CreateLine(TopLeftChar, HorizontalChar, TopRightChar));
    m_processSummaryPosition = m_textFile.FlushAndGetPosition();

    const std::string non_cell_line = CreateLine(VerticalChar, ' ', VerticalChar);
    WriteLine(non_cell_line);
    WriteLine(non_cell_line);

    if( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        ASSERT(m_processSummary.GetNumberLevels() > 0);

        WriteLine(non_cell_line);

        m_cellsTopBorderLine = CreateLine(LeftVerticalCellSeparatorChar, HorizontalChar, RightVerticalCellSeparatorChar, TopCellSeparatorChar);
        WriteLine(m_cellsTopBorderLine);

        // create the cell formatter
        for( const size_t cell_width : m_cellWidths )
        {
            m_cellsFormatter.append(FormatText("%s%*s%%%ds%*s", VerticalCharText, static_cast<int>(CellPaddingWidth), "",
                                                                static_cast<int>(cell_width), static_cast<int>(CellPaddingWidth), ""));
        }

        m_cellsFormatter.append(VerticalCharText);

        const std::string empty_cell_text = FormatText(m_cellsFormatter.c_str(), "", "", "", "");
        WriteLine(empty_cell_text);

        m_cellsMiddleBorderLine = CreateLine(LeftVerticalCellSeparatorChar, HorizontalChar, RightVerticalCellSeparatorChar, MiddleCellSeparatorChar);
        WriteLine(m_cellsMiddleBorderLine);

        for( size_t level_number = 0; level_number < m_processSummary.GetNumberLevels(); ++level_number )
            WriteLine(empty_cell_text);

        WriteLine(CreateLine(BottomLeftChar, HorizontalChar, BottomRightChar, BottomCellSeparatorChar));
    }

    else
    {
        WriteLine(CreateLine(BottomLeftChar, HorizontalChar, BottomRightChar));
    }

    m_textFile.WriteLine();
}


void Listing::ProcessSummaryFormatter::WriteValues()
{
    ASSERT(m_processSummaryPosition.has_value());
    m_textFile.Seek(*m_processSummaryPosition, SEEK_SET);

    auto write_summary_line = [&](std::string&& summary_line)
    {
        SO::WideMakeExactLength(summary_line, std::max(m_tableWidth, SO::WideLength(summary_line) + 1));
        SO::WideSetChar(summary_line, SO::WideLength(summary_line) - 1, VerticalChar);
        WriteLine(summary_line);
    };

    // write out information on the number of records (or slices) read
    write_summary_line(FormatText("%s%*d %s Read   %d%% of input file", VerticalCharText,
                                  static_cast<int>(MaximumRecordWidth), static_cast<int>(m_processSummary.GetAttributesRead()),
                                  ( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records ) ? "Records" : "Slices",
                                  static_cast<int>(m_processSummary.GetPercentSourceRead())));

    // write out information on bad records (or slices)
    write_summary_line(FormatText("%s%*d Ignored: %*d Unknown   %*d Erased", VerticalCharText,
                                  static_cast<int>(MaximumRecordWidth),        static_cast<int>(m_processSummary.GetAttributesIgnored()),
                                  static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary.GetAttributesUnknown()),
                                  static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary.GetAttributesErased())));

    if( m_processSummary.GetAttributesType() == ProcessSummary::AttributesType::Records )
    {
        // write out information on messages
        write_summary_line(FormatText("%s%*d Messages:%*d E%*d W%*d User", VerticalCharText,
                                      static_cast<int>(MaximumRecordWidth),        static_cast<int>(m_processSummary.GetTotalMessages()),
                                      static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary.GetErrorMessages()),
                                      static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary.GetWarningMessages()),
                                      static_cast<int>(MaximumSmallerNumberWidth), static_cast<int>(m_processSummary.GetUserMessages())));

        // write the level summaries
        WriteLine(m_cellsTopBorderLine);

        WriteLine(FormatText(m_cellsFormatter.c_str(), LevelText, InputCaseText, BadStructText, LevelPostText));

        WriteLine(m_cellsMiddleBorderLine);

        for( size_t level_number = 0; level_number < m_processSummary.GetNumberLevels(); ++level_number )
        {
            WriteLine(FormatText(m_cellsFormatter.c_str(), IntToString(level_number + 1).c_str(),
                                                           IntToString(m_processSummary.GetCaseLevelsRead(level_number)).c_str(),
                                                           IntToString(m_processSummary.GetBadCaseLevelStructures(level_number)).c_str(),
                                                           IntToString(m_processSummary.GetLevelPostProcsExecuted(level_number)).c_str()));
        }
    }

    m_textFile.SeekToEnd();
}


template<typename T>
void Listing::ProcessSummaryFormatter::WriteLine(T&& text)
{
    ASSERT80(SO::WideLength(text) == m_tableWidth);

    m_textFile.WriteString(m_marginText);
    m_textFile.WriteLine(std::forward<T>(text));
}


std::string Listing::ProcessSummaryFormatter::CreateLine(const wchar_t left_char, const wchar_t middle_char, const wchar_t right_char, const wchar_t cell_char/* = 0*/)
{
    std::wstring line(m_tableWidth, middle_char);
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

    return UTF8_TODO::GetUtf8(line);
}



// --------------------------------------------------------------------------
// TextLister
// --------------------------------------------------------------------------

Listing::TextLister::TextLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, const bool append, const PFF& pff)
    :   Lister(std::move(process_summary)),
        m_textFile(OpenListingFile(file_path, append)),
        m_listingWidth(pff.GetListingWidth()),
        m_wrapMessages(pff.GetMessageWrap()),
        m_messagesAreFromCase(false),
        m_writeProcessSummaryAndMessages(pff.GetAppType() != APPTYPE::ENTRY_TYPE)
{    
}


Listing::TextLister::~TextLister()
{
}


void Listing::TextLister::WriteHeader(const std::vector<HeaderAttribute>& header_attributes)
{
#define DescriptionFormatter "%-15s"

    // write out the header attributes
    for( const HeaderAttribute& header_attribute : header_attributes )
    {
        m_textFile->WriteFormattedString(DescriptionFormatter " ", header_attribute.description.c_str());

        if( header_attribute.secondary_description.has_value() )
            m_textFile->WriteFormattedString("%s=", header_attribute.secondary_description->c_str());

        if( std::holds_alternative<std::string>(header_attribute.value) )
        {
            m_textFile->WriteLine(std::get<std::string>(header_attribute.value));
        }

        else
        {
            m_textFile->WriteLine(std::get<ConnectionString>(header_attribute.value).GetName(DataRepositoryNameType::ForListing));
        }
    }

    // write out the date
    m_textFile->WriteFormattedLine("\n" DescriptionFormatter " %s", "Date", DateTime::LocalDateString().c_str());

    const std::string time = DateTime::LocalTimeString();
    m_textFile->WriteFormattedLine(DescriptionFormatter " %s", "Start Time", time.c_str());
    m_textFile->WriteFormattedString(DescriptionFormatter " %s", "End Time", time.c_str());
    m_endTimePosition = m_textFile->FlushAndGetPosition() - time.length();

    m_textFile->WriteLine("\n");

    if( !m_writeProcessSummaryAndMessages )
        return;

    m_textFile->WriteLine("CSPro Process Summary\n");

    m_processSummaryFormatter = std::make_unique<ProcessSummaryFormatter>(*m_processSummary, *m_textFile);
    m_processSummaryFormatter->WriteShell();

    m_textFile->WriteLine("Process Messages");
}


void Listing::TextLister::WriteMessages(const Messages& messages)
{
    constexpr const char* TypeCodes[] =
    {
        "A",
        "E",
        "W",
        "U"
    };

    // write the message source
    m_textFile->WriteString("\n*** ");

    if( m_messagesAreFromCase )
    {
        // make sure that all messages have the same level key
        const std::string& level_key = messages.messages.front().level_key;
        ASSERT(static_cast<size_t>(std::count_if(messages.messages.cbegin(), messages.messages.cend(),
                                                 [&](const Message& message) { return ( message.level_key == level_key ); })) == messages.messages.size());

        auto write_key_in_brackets = [&](const std::string& key_text)
        {
            m_textFile->WriteString("[");

            // when newlines are present in the key, replace them with ␤
            if( SO::ContainsNewlineCharacter(key_text) )
            {
                m_textFile->WriteString(NewlineSubstitutor::NewlineToUnicodeNL(key_text));
            }

            else
            {
                m_textFile->WriteString(key_text);
            }

            m_textFile->WriteString("]");
        };

        m_textFile->WriteString("Case ");

        write_key_in_brackets(messages.source);

        if( !level_key.empty() )
            write_key_in_brackets(level_key);
    }

    else
    {
        ASSERT(!SO::ContainsNewlineCharacter(messages.source));

        m_textFile->WriteString(messages.source);
    }

    // write the number of messages
    const auto [num_errors, num_warnings, num_user_messages, num_total_messages] = CountMessages();

    m_textFile->WriteFormattedLine(" has %d message%s (%d E / %d W / %d U)",
                                   static_cast<int>(num_total_messages), PluralizeWord(num_total_messages),
                                   static_cast<int>(num_errors), static_cast<int>(num_warnings), static_cast<int>(num_user_messages));

    constexpr size_t MessageNumberPadding = 14; // the padding, the type, and the number
    const size_t maximum_message_width = m_listingWidth - MessageNumberPadding;

    // write the messages
    for( const Message& message : messages.messages )
    {
        // write a message
        if( message.details.has_value() )
        {
            ASSERT(static_cast<size_t>(message.details->type) < _countof(TypeCodes));
            const char* const type_code = TypeCodes[static_cast<size_t>(message.details->type)];

            m_textFile->WriteFormattedString("    %s %7d ", type_code, message.details->number);

            // write the entire message or, if necessary, wrap it on multiple lines
            if( !SO::ContainsNewlineCharacter(*message.text) && SO::WideLength(*message.text) <= maximum_message_width )
            {
                m_textFile->WriteLine(*message.text);
            }

            else
            {
                bool add_padding = false;

                for( const std::wstring& line : SO::WrapText(UTF8_TODO::GetWide(*message.text), maximum_message_width) )
                {
                    ASSERT(!SO::ContainsNewlineCharacter(line));

                    if( add_padding )
                    {
                        m_textFile->WriteFormattedString("%*s", static_cast<int>(MessageNumberPadding), "");
                    }

                    else
                    {
                        add_padding = true;
                    }

                    m_textFile->WriteLine(UTF8_TODO::GetUtf8(line));
                }
            }
        }

        // there is no special formatting for text coming from the write function
        else
        {
            m_textFile->WriteLine(*message.text);
        }
    }
}


void Listing::TextLister::ProcessCaseSource(const Case* const data_case)
{
    m_messagesAreFromCase = ( data_case != nullptr );
}


void Listing::TextLister::WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries)
{
    ASSERT(!message_summaries.empty());

    const MessageSummary::Type& message_summaries_type = message_summaries.front().type;
    const char* const type_text = ( message_summaries_type == MessageSummary::Type::System )       ? "System" :
                                  ( message_summaries_type == MessageSummary::Type::UserNumbered ) ? "User numbered" :
                                                                                                     "User unnumbered";

    // write the header
    m_textFile->WriteFormattedLine("\n%s messages:\n", type_text);

    // calculate the width of each column
    constexpr size_t ColumnPadding = 2;
    constexpr size_t MaximumPercentWidth = 5;

    const size_t maximum_message_text_width = m_listingWidth - MaximumMessageNumberWidth - MaximumRecordWidth -
                                              MaximumPercentWidth - MaximumRecordWidth - ( 4 * ColumnPadding );
    ASSERT(maximum_message_text_width > 0 && maximum_message_text_width < m_listingWidth);

    auto write_formatted_line = [&](const char* const number, const char* const frequency, const char* const percent,
                                    std::variant<const char*, std::string> text, const char* const denom)
    {
        const char* text_content;
        size_t text_spacing;

        if( std::holds_alternative<const char*>(text) )
        {
            text_content = std::get<const char*>(text);
            text_spacing = maximum_message_text_width;
        }

        else
        {
            SO::WideMakeExactLength(std::get<std::string>(text), maximum_message_text_width);
            text_content = std::get<std::string>(text).c_str();
            text_spacing = std::get<std::string>(text).length();
        }

        ASSERT(!SO::ContainsNewlineCharacter(std::string_view(text_content)));

        m_textFile->WriteFormattedLine("%*s  %*s  %*s  %-*s  %*s",
                                       static_cast<int>(MaximumMessageNumberWidth), number,
                                       static_cast<int>(MaximumRecordWidth), frequency,
                                       static_cast<int>(MaximumPercentWidth), percent,
                                       static_cast<int>(text_spacing), text_content,
                                       static_cast<int>(MaximumRecordWidth), denom);
    };

    const bool use_denoms = ( message_summaries_type != MessageSummary::Type::System );
    const char* const number_heading = ( message_summaries_type == MessageSummary::Type::UserUnnumbered ) ? "Line" : "Number";
    constexpr const char* frequency_heading = "Freq";
    const char* const percent_heading = use_denoms ? "  %  " : "";
    constexpr const char* message_heading = "Message Text";
    const char* const denom_heading = use_denoms ? "Denom" : "";

    const size_t percent_heading_length = strlen(percent_heading);
    const size_t denom_heading_length = strlen(denom_heading);

    write_formatted_line(number_heading, frequency_heading, percent_heading, message_heading, denom_heading);

    write_formatted_line(SO::GetDashedLine(strlen(number_heading)),
                         SO::GetDashedLine(strlen(frequency_heading)),
                         SO::GetDashedLine(percent_heading_length),
                         SO::GetDashedLine(strlen(message_heading)),
                         SO::GetDashedLine(denom_heading_length));

    // write the messages
    const char* const blank_percent_denom_text = use_denoms ? "-" : "";
    std::string percent_text;
    std::string denom_text;
    std::vector<std::string> wrapped_lines;

    for( const MessageSummary& message_summary : message_summaries )
    {
        const std::string* first_line = &message_summary.message_text.GetString();

        if( SO::ContainsNewlineCharacter(*first_line) || SO::WideLength(*first_line) > maximum_message_text_width )
        {
            wrapped_lines = UTF8_TODO::GetUtf8(SO::WrapText(UTF8_TODO::GetWide(*first_line), maximum_message_text_width));
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

        // write the first line
        write_formatted_line(IntToString(message_number_for_display).c_str(),
                             IntToString(message_summary.frequency).c_str(),
                             has_a_denom ? percent_text.c_str() : blank_percent_denom_text,
                             *first_line,
                             has_a_denom ? denom_text.c_str() : blank_percent_denom_text);

        // write any wrapped lines
        for( size_t i = 1; i < wrapped_lines.size(); ++i )
            write_formatted_line("", "", "", std::move(wrapped_lines[i]), "");
    }
}


void Listing::TextLister::WriteWarningAboutApplicationErrors(const std::string& application_errors_path)
{
    m_textFile->WriteFormattedLine("\n\nA compilation error file (%s) has been created for your application.",
                                   PortableFunctions::PathGetFilename(application_errors_path).c_str());
}


void Listing::TextLister::UpdateProcessSummary()
{
    if( m_processSummaryFormatter != nullptr )
        m_processSummaryFormatter->WriteValues();
}


void Listing::TextLister::WriteFooter()
{
    m_textFile->WriteLine("\n\nCSPro Executor Normal End");

    // write out a dashed lined (60 and 231 come from work on 20100316)
    m_textFile->WriteLine(SO::GetDashedLine(std::max<size_t>(60, std::min<size_t>(m_listingWidth, 231))));

    // update the end time
    if( m_endTimePosition.has_value() )
    {
        m_textFile->Seek(*m_endTimePosition, SEEK_SET);
        m_textFile->WriteString(DateTime::LocalTimeString());
        m_textFile->SeekToEnd();
    }
}


std::optional<std::tuple<bool, Listing::ListingType, void*>> Listing::TextLister::GetFrequencyPrinter()
{
    return std::tuple<bool, Listing::ListingType, void*>(true, ListingType::Text, m_textFile.get());
}
