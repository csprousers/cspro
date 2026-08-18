#include "StdAfx.h"
#include "TextFile.h"
#include "FileIO.h"
#include <zDataO/ConnectionStringProperties.h>


// --------------------------------------------------------------------------
// FileIO::TextFile
// --------------------------------------------------------------------------

void FileIO::TextFile::ParsePropertiesWorker(PropertyRetriever& property_retriever, TextEncoding* const text_encoding, bool* const write_newline_as_crlf)
{
    // encoding
    if( text_encoding != nullptr )
    {
        const std::optional<std::string> encoding_override = property_retriever.GetProperty(CSProperty::encoding);

        if( encoding_override.has_value() )
        {
            if( SO::EqualsNoCase(*encoding_override, CSValue::ANSI) )
            {
                *text_encoding = TextEncoding::Type::Ansi;
            }

            else if( SO::EqualsNoCase(*encoding_override, CSValue::UTF_8) )
            {
                *text_encoding = TextEncoding::Type::Utf8;
            }

            else if( SO::EqualsNoCase(*encoding_override, CSValue::UTF_8_BOM) )
            {
                *text_encoding = TextEncoding::Type::Utf8Bom;
            }

            else // TEXT_ENCODING_TODO allow UTF-16
            {
                property_retriever.OnInvalidPropertyValue(CSProperty::encoding, *encoding_override);
            }
        }
    }

    // newline
    if( write_newline_as_crlf != nullptr )
    {
        const std::optional<std::string> newline_override = property_retriever.GetProperty(CSProperty::newline);

        if( newline_override.has_value() )
        {
            if( SO::EqualsNoCase(*newline_override, CSValue::CRLF) )
            {
                *write_newline_as_crlf = true;
            }

            else if( SO::EqualsNoCase(*newline_override, CSValue::LF) )
            {
                *write_newline_as_crlf = false;
            }

            else
            {
                property_retriever.OnInvalidPropertyValue(CSProperty::newline, *newline_override);
            }
        }
    }
}


FileIO::TextFile& FileIO::TextFile::OpenForTextReading(InterfaceString file_path, const int share_flag/* = INT_MIN*/)
{
    OpenForReading(std::move(file_path), share_flag);

    // process a potential BOM and create an encoding converter
    m_textEncoding.UpdateEncoding(m_file);
    m_textConverter = m_textEncoding.CreateConverter();

    return *this;
}


FileIO::TextFile& FileIO::TextFile::OpenForTextWritingCreate(InterfaceString file_path, const int share_flag/* = INT_MIN*/)
{
    OpenForWritingCreate(std::move(file_path), share_flag);

    // write a potential BOM
    if( m_textEncoding.UsesBom() )
        Write(m_textEncoding.GetBom());

    // create an encoding converter
    m_textConverter = m_textEncoding.CreateConverter();

    return *this;
}


FileIO::TextFile& FileIO::TextFile::OpenForTextWritingAppend(InterfaceString file_path, const int share_flag/* = INT_MIN*/)
{
    Open(std::move(file_path), "rb+", share_flag);

    // create an encoding converter
    m_textConverter = m_textEncoding.CreateConverter();

    SeekToEnd();

    return *this;
}


FileIO::TextFile& FileIO::TextFile::OpenForTextWriting(InterfaceString file_path, const bool append, const int share_flag/* = INT_MIN*/)
{
    return ( append && PortableFunctions::FileIsRegular(file_path) ) ? OpenForTextWritingAppend(std::move(file_path), share_flag) :
                                                                       OpenForTextWritingCreate(std::move(file_path), share_flag);
}


int64_t FileIO::TextFile::FlushAndGetPosition()
{
    ASSERT(IsOpen());

    if( fflush(m_file) != 0 )
        throw FileIO::Exception("There was an error flushing the file: %s", GetPath().c_str());

    return GetPosition();
}


bool FileIO::TextFile::ReadLine(std::string& line)
{
    int ch = fgetc(m_file);

    if( ch == EOF )
        return false;

    char* current_line_itr = line.data();
    const char* const current_line_end = current_line_itr + line.length();

    do
    {
        if( ch == '\n' )
        {
            break;
        }

        else if( ch == '\r' ) // TEXT_ENCODING_TODO need to read past two characters to process \r\n pairs for the UTF-16...formats
        {
            // process \r\n pairs
            ch = fgetc(m_file);

            if( ch != '\n' )
                ungetc(ch, m_file);

            break;
        }

        // add the character to the initial line's buffer...
        else if( current_line_itr < current_line_end )
        {
            *current_line_itr = static_cast<char>(ch);
            ++current_line_itr;
        }

        // ...or to the end
        else
        {
            line.push_back(static_cast<char>(ch));
        }

        ch = fgetc(m_file);

    } while( ch != EOF );

    // if the initial line's buffer was used entirely, resize the string
    if( current_line_itr != current_line_end )
    {
        ASSERT(current_line_end == line.data() + line.length());
        line.resize(current_line_itr - line.data());
    }

    if( m_textConverter != nullptr )
        line = m_textConverter->ToUtf8(line);

    return true;
}


FileIO::TextFile& FileIO::TextFile::WriteLine()
{
    if( m_writeNewlineAsCRLF )
    {
        File::Write(SO::Newline_crlf_sv.data(), SO::Newline_crlf_sv.length());
    }

    else
    {
        File::Write(SO::Newline_lf_sv.data(), SO::Newline_lf_sv.length());
    }

    return *this;
}


void FileIO::TextFile::WriteStringEnsuringCRLF(const std::string_view text_sv, const size_t newline_pos)
{
    ASSERT(m_writeNewlineAsCRLF &&
           newline_pos != std::string_view::npos &&
           text_sv.find('\n') == newline_pos);

    // write the block up to this newline
    if( newline_pos > 0 )
    {
        const size_t block_end_pos = ( text_sv[newline_pos - 1] == '\r' ) ? ( newline_pos - 1 ) :
                                                                            newline_pos;
        if( block_end_pos != 0 )
            WriteStringInCorrectEncoding(std::string_view(text_sv.data(), block_end_pos));
    }

    // write the newline
    WriteLine();

    // write the rest of the string
    WriteString(text_sv.substr(newline_pos + 1));
}


bool FileIO::TextFile::OutputStreamRequiresCRLFHandling() const
{
    ASSERT(m_textEncoding.IsUtf8()); // TEXT_ENCODING_TODO need to handle non-UTF-8 text

    return m_writeNewlineAsCRLF;
}
