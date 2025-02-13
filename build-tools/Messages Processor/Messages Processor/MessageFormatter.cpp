#include "stdafx.h"
#include "Main.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/imsaStr.h>


namespace
{
    constexpr int MaxMessageNumberDigits = 6;
}


void MessageFormatter::FormatMessageFiles()
{
    for( const std::wstring& message_filename : MessageLoader::GetMessageFilenames(true) )
    {
        // read the contents of the file, keeping track of blank/comment/language change lines
        std::map<int, std::vector<std::wstring>> lines_before_message_map;
        std::vector<std::wstring> current_lines_before_message;
        std::vector<int> message_numbers;

        SO::ForeachLine(FileIO::ReadText(message_filename), true, 
            [&](wstring_view line_sv)
            {
                if( line_sv.empty() || 
                    line_sv.front() == '{' || line_sv.front() == '/' ||
                    SO::StartsWith(line_sv, L"Language=") )
                {
                    current_lines_before_message.emplace_back(line_sv);
                }

                else
                {
                    ASSERT(std::iswdigit(line_sv.front()));
                    const int message_number = message_numbers.emplace_back(static_cast<int>(StringToNumber(line_sv.substr(0, SO::FindFirstWhitespace(line_sv)))));
                    ASSERT(message_number > 0 && message_number < std::numeric_limits<int>::max());

                    if( !current_lines_before_message.empty() )
                    {
                        lines_before_message_map.try_emplace(message_number, current_lines_before_message);
                        current_lines_before_message.clear();
                    }
                }

                return true;
            });

        ASSERT(current_lines_before_message.empty());


        // parse the messages
        TextSourceExternal system_message_text_source(message_filename);
        MessageFile message_file;
        message_file.Load(system_message_text_source, LogicSettings::Version::V8_0);


        // format the messages
        std::vector<std::wstring> formatted_message_lines;

        for( const int message_number : message_numbers )
        {
            // write any lines prior to the message
            const auto& lines_before_message_lookup = lines_before_message_map.find(message_number);

            if( lines_before_message_lookup != lines_before_message_map.cend() )
            {
                for( std::wstring line : lines_before_message_lookup->second )
                {
                    // format comments to properly line up with message numbers
                    SO::MakeTrim(line);

                    const size_t comment_length = ( line.find('{') == 0 )   ? 1 :
                                                  ( line.find(L"/*") == 0 ) ? 2 :
                                                                              0;

                    if( comment_length != 0 )
                    {
                        line = SO::Trim(line.substr(comment_length, line.length() - 2 * comment_length));

                        while( true )
                        {
                            const size_t initial_line_length = line.length();

                            SO::MakeTrimLeft(line, '-');
                            SO::MakeTrimLeft(line);

                            if( initial_line_length == line.length() )
                                break;
                        }

                        line = L"/* --- " + line + L" */";
                    }

                    formatted_message_lines.emplace_back(std::move(line));
                }
            }

            // write the message text, escaping it only as necessary
            std::wstring message_text = message_file.GetMessageText(message_number);
            ASSERT(!message_text.empty());
            
            if( message_text.front() == '\'' || message_text.front() == '"' || SO::ContainsNewlineCharacter(message_text) )
                message_text = L"\"" + Encoders::ToEscapedString(message_text, false) + L"\"";

            formatted_message_lines.emplace_back(FormatTextCS2WS(L"%-*d %s", MaxMessageNumberDigits, message_number, message_text.c_str()));
        }

        
        // write the formatted message text
        const std::wstring formatted_message_text = SO::CreateSingleString(formatted_message_lines, L"\r\n") + L"\r\n";
        FileIO::WriteText(message_filename, formatted_message_text, true);
    }
}
