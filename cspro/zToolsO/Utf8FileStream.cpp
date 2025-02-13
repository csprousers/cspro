#include "StdAfx.h"
#include "Utf8FileStream.h"


namespace
{
    class Utf8InputFileStreamBuffer : public std::wstreambuf
    {
        constexpr static size_t MaxUtf8BytesSequence = 4;
        constexpr static size_t ReadBufferSize = 1024;
        constexpr static size_t ReadBufferSizeWithUnicodeOverflow = ReadBufferSize + MaxUtf8BytesSequence - 1;
        static_assert(Utf8BOM_sv.length() <= ReadBufferSize);

    public:
        Utf8InputFileStreamBuffer(std::wstring filename, FileIO::FileAndSize&& file_and_size);
        ~Utf8InputFileStreamBuffer();

        int GetPercentRead() const;

    protected:
        int_type underflow() override;

        pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

    private:
        void ReadBytes(char* buffer, size_t bytes);

        void ReadBytesAndConvert(char* buffer_start, size_t max_bytes_to_read);

    private:
        std::wstring m_filename;
        FILE* m_file;
        size_t m_fileSize;
        size_t m_fileSizeRemaining;
        size_t m_filePositionStart;
        char m_utf8Buffer[ReadBufferSizeWithUnicodeOverflow];
        wchar_t m_wideBuffer[ReadBufferSize];
    };


    Utf8InputFileStreamBuffer::Utf8InputFileStreamBuffer(std::wstring filename, FileIO::FileAndSize&& file_and_size)
        :   m_filename(std::move(filename)),
            m_file(file_and_size.file),
            m_fileSize((size_t)file_and_size.size),
            m_fileSizeRemaining(m_fileSize),
            m_filePositionStart(0)
    {
        ASSERT(ftell(m_file) == 0);
        ASSERT(file_and_size.size >= 0);

        // skip past the BOM if necessary
        if( m_fileSizeRemaining >= Utf8BOM_sv.length() )
        {
            ReadBytes(m_utf8Buffer, Utf8BOM_sv.length());

            if( HasUtf8BOM(m_utf8Buffer, Utf8BOM_sv.length()) )
            {
                m_filePositionStart = Utf8BOM_sv.length();
            }

            else
            {
                // if this is not a BOM, read more bytes and convert this set
                ReadBytesAndConvert(m_utf8Buffer + Utf8BOM_sv.length(), ReadBufferSize - Utf8BOM_sv.length());
            }
        }
    }

    Utf8InputFileStreamBuffer::~Utf8InputFileStreamBuffer()
    {
        fclose(m_file);
    }


    int Utf8InputFileStreamBuffer::GetPercentRead() const
    {
        return 100 - CreatePercent(m_fileSizeRemaining, m_fileSize);
    }


    Utf8InputFileStreamBuffer::int_type Utf8InputFileStreamBuffer::underflow()
    {
        if( m_fileSizeRemaining == 0 )
            return traits_type::eof();

        ReadBytesAndConvert(m_utf8Buffer, ReadBufferSize);

        return m_wideBuffer[0];
    }


    Utf8InputFileStreamBuffer::pos_type Utf8InputFileStreamBuffer::seekpos(pos_type sp, std::ios_base::openmode/* which = std::ios_base::in | std::ios_base::out*/)
    {
        std::streamoff stream_offset = sp;
        long file_offset = static_cast<long>(m_filePositionStart) + static_cast<long>(stream_offset);

        // this has only been tested to go to the beginning of the file
        ASSERT(file_offset == static_cast<long>(m_filePositionStart));
        
        if( file_offset < static_cast<long>(m_fileSize) && fseek(m_file, static_cast<long>(file_offset), SEEK_SET) == 0 )
        {
            m_fileSizeRemaining = m_fileSize - file_offset;
            return sp;
        }

        throw FileIO::Exception(_T("Could not seek to position %d in file %s."), (int)file_offset, PortableFunctions::PathGetFilename(m_filename));
    }


    void Utf8InputFileStreamBuffer::ReadBytes(char* buffer, size_t bytes)
    {
        ASSERT(bytes <= m_fileSizeRemaining);

        if( fread(buffer, 1, bytes, m_file) != bytes )
            throw FileIO::Exception(_T("The file %s could not be fully read."), PortableFunctions::PathGetFilename(m_filename));

        m_fileSizeRemaining -= bytes;
    }


    void Utf8InputFileStreamBuffer::ReadBytesAndConvert(char* buffer_start_read_pos, size_t max_bytes_to_read)
    {
        ASSERT(buffer_start_read_pos >= m_utf8Buffer);
        ASSERT(( buffer_start_read_pos + max_bytes_to_read ) <= ( m_utf8Buffer + ReadBufferSize ));

        size_t bytes_in_buffer = std::min(max_bytes_to_read, m_fileSizeRemaining);
        ReadBytes(buffer_start_read_pos, bytes_in_buffer);

        // when the buffer does not start at the beginning, which will only happen when processing
        // the bytes read in for the BOM check, adjust the figures
        if( buffer_start_read_pos != m_utf8Buffer )
        {
            ASSERT(m_utf8Buffer == ( buffer_start_read_pos - Utf8BOM_sv.length() ));
            bytes_in_buffer += Utf8BOM_sv.length();
        }

        ASSERT(bytes_in_buffer > 0);

        // if the last character read is in the middle of a UTF-8 sequence, read the whole sequence
        if( m_fileSizeRemaining > 0 )
        {
            char* last_char_in_buffer = m_utf8Buffer + bytes_in_buffer - 1;

            if( ( *last_char_in_buffer & 0x80 ) == 0x80 )
            {
                // find the beginning of the current sequence
                const char* start_utf_sequence = last_char_in_buffer;

                while( ( *start_utf_sequence & 0xC0 ) != 0xC0 && start_utf_sequence > m_utf8Buffer )
                    --start_utf_sequence;

                ASSERT(( *start_utf_sequence & 0xC0 ) == 0xC0);

                size_t bytes_in_sequence = ( ( *start_utf_sequence & 0xF0 ) == 0xF0 ) ? 4 :
                                           ( ( *start_utf_sequence & 0xE0 ) == 0xE0 ) ? 3 : 
                                                                                        2;

                size_t current_sequence_bytes_read = last_char_in_buffer + 1 - start_utf_sequence;
                size_t current_sequence_bytes_remaining = bytes_in_sequence - current_sequence_bytes_read;

                if( current_sequence_bytes_remaining < MaxUtf8BytesSequence && current_sequence_bytes_remaining <= m_fileSizeRemaining )
                {
                    ReadBytes(last_char_in_buffer + 1, current_sequence_bytes_remaining);
                    bytes_in_buffer += current_sequence_bytes_remaining;
                }

                else
                {
                    ASSERT(false);
                }
            }
        }

        // convert the bytes read
        int wide_chars = UTF8Convert::UTF8BufferToWideBuffer(m_utf8Buffer, bytes_in_buffer, m_wideBuffer, _countof(m_wideBuffer));

        setg(m_wideBuffer, m_wideBuffer, m_wideBuffer + wide_chars);
    }
}



Utf8InputFileStream::Utf8InputFileStream(std::unique_ptr<std::wstreambuf> stream_buffer)
    :   std::wistream(stream_buffer.get()),
        m_streamBuffer(std::move(stream_buffer))
{
}


Utf8InputFileStream::Utf8InputFileStream(std::wstring filename, FileIO::FileAndSize file_and_size)
    :   Utf8InputFileStream(std::make_unique<Utf8InputFileStreamBuffer>(std::move(filename), std::move(file_and_size)))
{
}


int Utf8InputFileStream::GetPercentRead() const
{
    return assert_cast<Utf8InputFileStreamBuffer*>(m_streamBuffer.get())->GetPercentRead();
}
