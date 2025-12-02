#include "stdafx.h"
#include "Main.h"
#include <zToolsO/File.h>


MessageFileAuditor::MessageFileAuditor()
    :   loading_english_messages(false)
{
}


void MessageFileAuditor::LoadedMessageNumber(int message_number)
{
    if( loading_english_messages )
        ordered_english_message_numbers.emplace_back(message_number);
}


void MessageFileAuditor::DoAudit()
{
    MessageLoader::LoadMessageFiles(*this, false, &loading_english_messages);       

    // make sure that English is the current language
    ChangeLanguage("EN");

    FileIO::TextFile all_messages_file;
    all_messages_file.OpenForTextWritingCreate("CSProRuntime Messages Audit - All Messages.csv");

    FileIO::TextFile missing_messages_file;
    missing_messages_file.OpenForTextWritingCreate("CSProRuntime Messages Audit - Missing Messages.csv");

    FileIO::TextFile invalid_format_specifiers_file;
    invalid_format_specifiers_file.OpenForTextWritingCreate("CSProRuntime Messages Audit - Invalid Format Specifiers.txt");

    // write out the headers
    std::string all_messages_header = Encoders::ToCsv("Message Number");
    std::string missing_messages_header = Encoders::ToCsv("Message Text");

    for( const LanguageSet& language_set : m_languageSets )
    {
        all_messages_header.push_back(',');
        all_messages_header.append(Encoders::ToCsv(language_set.language_name));

        missing_messages_header.push_back(',');
        missing_messages_header.append(Encoders::ToCsv(language_set.language_name));
    }

    all_messages_file.WriteLine(all_messages_header);
    missing_messages_file.WriteLine(missing_messages_header);

    // process each message in the English message file
    for( const int message_number : ordered_english_message_numbers )
    {
        const std::string message_number_text = IntToString(message_number);

        const SharableString english_message_text = GetMessageText(message_number);
        ASSERT(english_message_text.IsSet());

        std::string all_messages_row = Encoders::ToCsv(message_number_text);
        std::string missing_messages_row = Encoders::ToCsv(*english_message_text);

        bool message_was_missing = false;

        for( const LanguageSet& language_set : m_languageSets )
        {
            std::string message_text;
            const auto& lookup = language_set.numbered_messages.find(message_number);

            if( lookup == language_set.numbered_messages.end() )
            {
                message_was_missing = true;
                missing_messages_row.push_back(',');
                missing_messages_row.append(message_number_text);
            }

            else
            {
                message_text = *lookup->second;
                missing_messages_row.push_back(',');

                if( !CheckFormatSpecifiers(*english_message_text, message_text) )
                {
                    invalid_format_specifiers_file.WriteLine(language_set.language_name);
                    invalid_format_specifiers_file.WriteLine(*english_message_text);
                    invalid_format_specifiers_file.WriteLine(message_text);
                    invalid_format_specifiers_file.WriteLine();
                }
            }

            all_messages_row.push_back(',');
            all_messages_row.append(Encoders::ToCsv(message_text));
        }

        all_messages_file.WriteLine(all_messages_row);

        if( message_was_missing )
            missing_messages_file.WriteLine(missing_messages_row);
    }

    all_messages_file.Close();
    missing_messages_file.Close();
    invalid_format_specifiers_file.Close();
}


bool MessageFileAuditor::CheckFormatSpecifiers(const std::string& english_message_text, const std::string& message_text)
{
    // make sure that the format specifiers are in the same order as the English version
    const std::vector<std::string> english_formatters = ExtractFormatSpecifiers(english_message_text);
    const std::vector<std::string> other_language_formatters = ExtractFormatSpecifiers(message_text);
    
    if( english_formatters.size() != other_language_formatters.size() )
        return false;

    for( size_t i = 0; i < english_formatters.size(); i++ )
    {
        if( english_formatters[i] != other_language_formatters[i] )
            return false;
    }

    return true;
}


std::vector<std::string> MessageFileAuditor::ExtractFormatSpecifiers(const std::string& message_text)
{
    std::vector<std::string> formatters;

    bool in_formatter = false;

    for( size_t i = 0; i < message_text.size(); i++ )
    {
        const char ch = message_text[i];

        if( ch == '%' )
        {
            if( ( i + 1 ) < message_text.size() && message_text[i + 1] != '%' )
            {
                formatters.emplace_back();
                in_formatter = true;
            }
        }

        if( in_formatter )
        {
            formatters.back().push_back(ch);

            if( isalpha(ch) )
                in_formatter = false;
        }
    }

    return formatters;
}
