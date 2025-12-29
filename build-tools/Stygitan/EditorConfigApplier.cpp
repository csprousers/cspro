#include "StdAfx.h"
#include "EditorConfigApplier.h"


const BinaryBlock* EditorConfig::Applier::Process(const std::string& file_path, const EditorConfig::Options& options)
{
    ASSERT(options.IsDefined());

    auto lookup = m_data.find(file_path);
    const std::tuple<int64_t, int64_t> file_size_and_modified_time = PortableFunctions::FileSizeAndModifiedTime(file_path);

    if( lookup == m_data.cend() ||
        lookup->second.file_size_and_modified_time != file_size_and_modified_time )
    {
        const BinaryBlock file_data = FileIO::ReadBinary(file_path);

        lookup = m_data.insert_or_assign(file_path,
                                         Data { file_size_and_modified_time, Process(file_data, options) }).first;
    }

    return lookup->second.processed_file.get();
}


std::unique_ptr<BinaryBlock> EditorConfig::Applier::Process(const BinaryBlock& file_data, const EditorConfig::Options& options)
{
    // first convert the binary data to UTF-8 text for processing
    std::string text(file_data.data<char>(), file_data.size());

    const TextEncoding::Type default_encoding_if_no_bom = 
        ( options.charset == EditorConfig::Charset::Latin1 ) ? TextEncoding::Type::Ansi : 
                                                               TextEncoding::Type::Utf8;
    const TextEncoding text_encoding(text, default_encoding_if_no_bom);

    text.erase(0, text_encoding.GetBomLength());

    std::unique_ptr<TextEncoding::Converter> text_converter = text_encoding.CreateConverter();

    if( text_converter != nullptr )
        text = text_converter->ToUtf8(text);

    // indent_style + indent_size
    if( options.indent_style == Indent::Space && options.indent_size.has_value() )
        SO::ConvertTabsToSpaces(text, 0, *options.indent_size);

    // end_of_line
    if( options.end_of_line.has_value() )
        ProcessEndOfLine(text, *options.end_of_line);

    // trim_trailing_whitespace
    if( options.trim_trailing_whitespace == true )
        ProcessTrimTrailingWhitespacePerLine(text);

    // stygitan_trim_final_newlines
    if( options.stygitan_trim_final_newlines == true )
        SO::MakeTrimRight(text);

    // insert_final_newline
    if( options.insert_final_newline == true )
        ProcessInsertFinalNewline(text, options.end_of_line);

    // charset
    std::string_view bom_sv;
    std::tie(bom_sv, text_converter) = ProcessCharset(text_encoding, options.charset);

    // potentially convert from UTF-8 to another encoding (e.g., UTF-8 -> ANSI)
    if( text_converter != nullptr )
        text = text_converter->FromUtf8(text);

    // construct the final converted text
    BinaryBlock converted_text(bom_sv.size() + text.size());
    char* converted_text_data = converted_text.data<char>();

    if( !bom_sv.empty() )
    {
        memcpy(converted_text_data, bom_sv.data(), bom_sv.size());
        converted_text_data += bom_sv.size();
    }

    memcpy(converted_text_data, text.data(), text.size());

    // if the contents are unchanged, return null
    if( file_data.size() == converted_text.size() &&
        memcmp(file_data.data(), converted_text.data(), file_data.size()) == 0 )
    {
        return nullptr;
    }

    return std::make_unique<BinaryBlock>(std::move(converted_text));
}


void EditorConfig::Applier::ProcessEndOfLine(std::string& text, const EndOfLine end_of_line)
{
    const char single_newline_ch = ( end_of_line == EndOfLine::LF ) ? '\n' :
                                   ( end_of_line == EndOfLine::CR ) ? '\r' :
                                                                      '\0';
    size_t newline_pos = std::string::npos;

    while( ( newline_pos = text.find_first_of(SO::Newline_crlf_sv, newline_pos + 1) ) != std::string::npos )
    {
        char& newline_ch = text[newline_pos];
        const bool is_rn = ( newline_ch == '\r' && text[newline_pos + 1] == '\n' );

        if( is_rn )
        {
            // convert \r\n -> \r or \n
            if( single_newline_ch != 0 )
            {
                newline_ch = single_newline_ch;
                text.erase(newline_pos + 1, 1);
            }

            else
            {
                ASSERT(end_of_line == EndOfLine::CRLF);
                ++newline_pos;
            }
        }

        // convert \r or \n -> \r\n
        else if( single_newline_ch == 0 )
        {
            text.replace(newline_pos, 1, SO::Newline_crlf_sv);
            ++newline_pos;
        }

        // ensure that the single newline character \r or \n is correct
        else
        {
            newline_ch = single_newline_ch;
        }
    }
}


void EditorConfig::Applier::ProcessTrimTrailingWhitespacePerLine(std::string& text)
{
    // modified from SO::ConvertTabsToSpacesWorker
    for( size_t i = 1; i < text.length(); ++i )
    {
        if( is_crlf(text[i]) )
        {
            const std::string_view text_up_to_this_crlf_sv(text.data(), i);
            const size_t spaces_at_end = text_up_to_this_crlf_sv.length() - SO::TrimRightSpace(text_up_to_this_crlf_sv).length();

            if( spaces_at_end > 0 )
            {
                text.erase(i - spaces_at_end, spaces_at_end);
                i -= spaces_at_end;
            }

            if( text[i + 1] == '\n' )
                ++i;
        }
    }
}


void EditorConfig::Applier::ProcessInsertFinalNewline(std::string& text, std::optional<EndOfLine> end_of_line)
{
    if( text.empty() || is_crlf(text.back()) )
        return;

    // if no end of line value is defined, use as its value the text's
    // first newline, defaulting to \n if there is no newline
    if( !end_of_line.has_value() )
    {
        const size_t r_pos = text.find('\r');
        const size_t n_pos = text.find('\n');

        end_of_line = ( ( r_pos + 1 ) == n_pos ) ? EndOfLine::CRLF :
                      ( r_pos < n_pos )          ? EndOfLine::CR :
                                                   EndOfLine::LF;
    }

    text.append(( *end_of_line == EndOfLine::LF ) ? SO::Newline_lf_sv :
                ( *end_of_line == EndOfLine::CR ) ? std::string_view("\r") :
                                                    SO::Newline_crlf_sv);
}


std::tuple<std::string_view, std::unique_ptr<TextEncoding::Converter>>
EditorConfig::Applier::ProcessCharset(const TextEncoding& text_encoding, const std::optional<Charset>& charset)
{
    const TextEncoding::Type type =
        ( !charset.has_value() )         ? text_encoding.GetType() :
        ( *charset == Charset::Latin1 )  ? TextEncoding::Type::Ansi :
        ( *charset == Charset::Utf8 )    ? TextEncoding::Type::Utf8 :
        ( *charset == Charset::Utf8Bom ) ? TextEncoding::Type::Utf8Bom :
        ( *charset == Charset::Utf16BE ) ? TextEncoding::Type::Utf16BE :
        ( *charset == Charset::Utf16LE ) ? TextEncoding::Type::Utf16LE :
                                           throw ProgrammingErrorException();

    return std::make_tuple(( type == TextEncoding::Type::Utf8Bom ) ? TextEncoding::Utf8Bom_sv :
                           ( type == TextEncoding::Type::Utf16LE ) ? TextEncoding::Utf16LEBom_sv :
                           ( type == TextEncoding::Type::Utf16BE ) ? TextEncoding::Utf16BEBom_sv :
                                                                     std::string_view(),
                           TextEncoding::CreateConverter(type));
}
