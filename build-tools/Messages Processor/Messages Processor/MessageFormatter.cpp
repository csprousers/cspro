#include "stdafx.h"
#include "Main.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ImsaStr.h>


namespace
{
    constexpr int MaxMessageNumberDigits = 6;
}


void MessageFormatter::FormatMessageFiles()
{
    for( const std::string& message_file_path : MessageLoader::GetMessageFilePaths(true) )
    {
        // read the contents of the file, keeping track of blank/comment/language change lines
        std::map<int, std::vector<std::string>> lines_before_message_map;
        std::vector<std::string> current_lines_before_message;
        std::vector<int> message_numbers;

        SO::ForeachLine(FileIO::ReadText(message_file_path), true,
            [&](const std::string_view line_sv)
            {
                if( line_sv.empty() ||
                    line_sv.front() == '{' || line_sv.front() == '/' ||
                    SO::StartsWith(line_sv, "Language=") )
                {
                    current_lines_before_message.emplace_back(line_sv);
                }

                else
                {
                    ASSERT(std::isdigit(line_sv.front()));
                    const int message_number = message_numbers.emplace_back(static_cast<int>(StringToNumber(UTF8_TODO::GetWide(line_sv.substr(0, SO::FindFirstWhitespace(line_sv))))));
                    ASSERT(message_number > 0 && message_number < std::numeric_limits<int>::max());

                    if( !current_lines_before_message.empty() )
                    {
                        lines_before_message_map.try_emplace(message_number, current_lines_before_message);
                        current_lines_before_message.clear();
                    }
                }
            });

        ASSERT(current_lines_before_message.size() == 1 && current_lines_before_message.front().empty());


        // parse the messages
        TextSourceExternal system_message_text_source(message_file_path);
        MessageFile message_file;
        message_file.Load(system_message_text_source, LogicSettings::Version::V8_0);


        // format the messages
        std::vector<std::string> formatted_message_lines;

        for( const int message_number : message_numbers )
        {
            // write any lines prior to the message
            const auto& lines_before_message_lookup = lines_before_message_map.find(message_number);

            if( lines_before_message_lookup != lines_before_message_map.cend() )
            {
                for( std::string line : lines_before_message_lookup->second )
                {
                    // format comments to properly line up with message numbers
                    SO::MakeTrim(line);

                    const size_t comment_length = ( line.find('{') == 0 )  ? 1 :
                                                  ( line.find("/*") == 0 ) ? 2 :
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

                        line = "/* --- " + line + " */";
                    }

                    formatted_message_lines.emplace_back(std::move(line));
                }
            }

            // write the message text, escaping it only as necessary
            std::string message_text = message_file.GetMessageText(message_number).Release();
            ASSERT(!message_text.empty());

            if( message_text.front() == '\'' || message_text.front() == '"' || SO::ContainsNewlineCharacter(message_text) )
                message_text = Encoders::ToLogicString(message_text);

            formatted_message_lines.emplace_back(FormatText("%-*d %s", MaxMessageNumberDigits, message_number, message_text.c_str()));
        }


        // write the formatted message text
        const std::string formatted_message_text = SO::CreateSingleString(formatted_message_lines, "\r\n") + "\r\n";
        FileIO::WriteText(message_file_path, formatted_message_text, true);
    }
}
