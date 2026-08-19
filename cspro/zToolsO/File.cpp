#include "StdAfx.h"
#include "File.h"
#include "FileIO.h"


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


void FileIO::File::Flush()
{
    ASSERT(IsOpen());

    if( fflush(m_file) != 0 )
        throw FileIO::Exception("There was an error flushing the file: %s", GetPath().c_str());
}


int64_t FileIO::File::FlushAndGetPosition()
{
    Flush();
    return GetPosition();
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
    {
        if( OutputStreamRequiresCRLFHandling() )
        {
            m_outputStreamBuffer = std::make_unique<OutputStreamBufferWithCRLFHandling>(*this);
        }

        else
        {
            m_outputStreamBuffer = std::make_unique<OutputStreamBuffer>(*this);
        }
    }

    return std::make_unique<std::ostream>(m_outputStreamBuffer.get());
}


bool FileIO::File::OutputStreamRequiresCRLFHandling() const
{
    return false;
}
