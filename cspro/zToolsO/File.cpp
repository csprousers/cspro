#include "StdAfx.h"
#include "File.h"
#include "FileIO.h"
#include <zDataO/ConnectionStringProperties.h>


// --------------------------------------------------------------------------
// FileIO::File::OutputStreamBuffer +
// FileIO::File::OutputStreamBufferWithCRLFHandling
// --------------------------------------------------------------------------

class FileIO::File::OutputStreamBuffer : public std::streambuf
{
public:
    OutputStreamBuffer(File& file)
        :   m_file(file)
    {
    }

protected:
    int_type overflow(int_type ch) override
    {
        if( fputc(ch, m_file.m_file) == EOF  )
            m_file.OnWriteError();

        return ch;
    }

private:
    File& m_file;
};


class FileIO::File::OutputStreamBufferWithCRLFHandling : public FileIO::File::OutputStreamBuffer
{
public:
    using OutputStreamBuffer::OutputStreamBuffer;

protected:
    int_type overflow(int_type ch) override
    {
        if( ch == '\n' && m_previousChar != '\r' )
            OutputStreamBuffer::overflow('\r');

        m_previousChar = ch;

        return OutputStreamBuffer::overflow(ch);
    }

private:
    int_type m_previousChar;
};



// --------------------------------------------------------------------------
// FileIO::File
// --------------------------------------------------------------------------

FileIO::File::File()
    :   m_file(nullptr),
        m_deleteFileOnError(false)
{
}


FileIO::File::File(File&& rhs) noexcept
    :   m_file(rhs.m_file),
        m_filePath(std::move(rhs.m_filePath)),
        m_deleteFileOnError(rhs.m_deleteFileOnError),
        m_outputStreamBuffer(std::move(rhs.m_outputStreamBuffer))
{
    rhs.m_file = nullptr;
}


FileIO::File::~File()
{
    if( m_file != nullptr )
    {
        fclose(m_file);

        if( m_deleteFileOnError )
            DeleteFile(m_filePath);
    }
}


FileIO::File& FileIO::File::Open(InterfaceString file_path, const char* const mode, const int share_flag/* = INT_MIN*/)
{
    if( IsOpen() )
    {
        ASSERT(false);
        throw FileIO::Exception("You must close '%s' before you can open: %s", m_filePath.c_str(), file_path.c_str_utf8());
    }

    m_file = PortableFunctions::FileOpen(file_path, mode, share_flag);

    if( m_file == nullptr )
    {
        if( strstr(mode, "r") != nullptr )
        {
            throw Exception::FileOpenError(file_path);
        }

        else
        {
            ASSERT(strstr(mode, "w") != nullptr);
            throw Exception::FileCreateError(file_path);
        }
    }

    m_filePath = file_path.Release<std::string>();

    return *this;
}


FileIO::File& FileIO::File::OpenForReading(InterfaceString file_path, const int share_flag/* = INT_MIN*/)
{
    return Open(std::move(file_path), "rb", share_flag);
}


FileIO::File& FileIO::File::OpenForWritingCreate(InterfaceString file_path, const int share_flag/* = INT_MIN*/)
{
    return Open(std::move(file_path), "wb", share_flag);
}


void FileIO::File::Close(const bool throw_exceptions)
{
    if( m_file == nullptr )
        return;

    const int close_result = fclose(m_file);

    m_file = nullptr;
    m_filePath.clear();

    if( throw_exceptions && close_result != 0 )
        throw FileIO::Exception("There was an error closing the file: " + m_filePath);
}


int64_t FileIO::File::GetPosition()
{
    ASSERT(IsOpen());

    return PortableFunctions::ftelli64(m_file);
}


void FileIO::File::DeleteFile(const std::string& file_path) noexcept
{
    ASSERT(!file_path.empty());

    try
    {
        PortableFunctions::FileDelete(file_path);
    }
    catch(...) { ASSERT(false); }
}


void FileIO::File::OnReadError()
{
    throw Exception::FileReadError(m_filePath);
}


void FileIO::File::OnWriteError()
{
    const std::string saved_file_path = m_filePath;

    if( m_deleteFileOnError )
    {
        Close(false);
        DeleteFile(saved_file_path);
    }

    throw Exception::FileNotFullyWritten(saved_file_path, false);
}


void FileIO::File::Seek(const int64_t offset, const int origin)
{
    ASSERT(IsOpen());

    if( PortableFunctions::fseeki64(m_file, offset, origin) != 0 )
    {
        throw FileIO::Exception("There was an error seeking to position '%s' in the file: %s",
                                IntToString(offset).c_str(),
                                m_filePath.c_str());
    }
}


std::unique_ptr<std::ostream> FileIO::File::GetOutputStream()
{
    ASSERT(IsOpen());

    if( m_outputStreamBuffer == nullptr )
        m_outputStreamBuffer = CreateOutputStream();

    return std::make_unique<std::ostream>(m_outputStreamBuffer.get());
}


std::unique_ptr<FileIO::File::OutputStreamBuffer> FileIO::File::CreateOutputStream()
{
    return std::make_unique<OutputStreamBuffer>(*this);
}



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


std::unique_ptr<FileIO::File::OutputStreamBuffer> FileIO::TextFile::CreateOutputStream()
{
    ASSERT(m_textEncoding.IsUtf8()); // TEXT_ENCODING_TODO need to handle non-UTF-8 text

    if( m_writeNewlineAsCRLF )
        return std::make_unique<OutputStreamBufferWithCRLFHandling>(*this);

    return File::CreateOutputStream();
}
