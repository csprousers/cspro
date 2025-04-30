#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/PropertyRetriever.h>
#include <zToolsO/TextEncoding.h>
#include <sstream>

namespace FileIO { class File; class TextFile; }


// the FileIO::File and FileIO::TextFile classes are wrappers around FILE functionality;
// all files are opened as binary, but FileIO::TextFile also has functionality, described
// in comments in the class, that can:
//     - convert UTF-8 text to other encoding formats
//     - process a BOM
//     - write newlines as either \n or \r\n

class CLASS_DECL_ZTOOLSO FileIO::File
{
public:
    File();
    File(const File& rhs) = delete;
    File(File&& rhs) noexcept;

    virtual ~File();

    File& operator=(const File& rhs) = delete;
    File& operator=(File&& rhs) = delete;

    // if set to true, the file is automatically deleted:
    // - if the file has not been closed manually prior to destruction
    // - on write errors
    File& SetDeleteFileOnError(bool delete_file) { m_deleteFileOnError = delete_file; return *this; }

    // indicates if a file is open
    bool IsOpen() const { return ( m_file != nullptr ); }

    // returns the path of the open file
    const std::string& GetPath() const { return m_filePath; }

    // opens a file, throwing an exception on error
    File& Open(InterfaceString file_path, const char* mode, int share_flag = INT_MIN);

    // opens an existing file for reading ("rb")
    File& OpenForReading(InterfaceString file_path, int share_flag = INT_MIN);

    // creates a new file for writing ("wb")
    File& OpenForWritingCreate(InterfaceString file_path, int share_flag = INT_MIN);

    // closes a file, throwing an exception on error
    void Close() { Close(true); }

    // returns the current file position
    int64_t GetPosition();

    // reads bytes up to the length specified, returning the number of bytes read
    size_t Read(void* buffer, size_t max_length);

    // reads bytes to the exact length specified, throwing an exception on error
    size_t ReadExact(void* buffer, size_t exact_length);

    // writes bytes, throwing an exception on error
    File& Write(const void* buffer, size_t length);

    // writes the characters of a string, throwing an exception on error;
    // note that \n characters are written simply as \n; to potentially write
    // such characters as \r\n, use the TextFile class and its WriteString method
    File& Write(const std::string& text)  { return Write(text.data(), text.length()); }
    File& Write(std::string_view text_sv) { return Write(text_sv.data(), text_sv.length()); }
    File& Write(const char* text)         { return Write(text, strlen(text)); }

    // writes the binary representation of a data type
    template<typename T>
    File& WriteBinary(const T& value);

    // seeks to a different position of the file, throwing an exception on error
    void Seek(int64_t offset, int origin);
    void SeekToEnd();

    // returns a std::ostream that can be used for outputting data;
    // only outputs are implemented so do not use this stream for random access
    std::unique_ptr<std::ostream> GetOutputStream();

protected:
    class OutputStreamBuffer;
    class OutputStreamBufferWithCRLFHandling;
    virtual std::unique_ptr<OutputStreamBuffer> CreateOutputStream();

private:
    void Close(bool throw_exceptions);

    static void DeleteFile(const std::string& file_path) noexcept;

    [[noreturn]] void OnReadError();
    [[noreturn]] void OnWriteError();

protected:
    FILE* m_file;

private:
    std::string m_filePath;
    bool m_deleteFileOnError;
    std::unique_ptr<OutputStreamBuffer> m_outputStreamBuffer;
};



class CLASS_DECL_ZTOOLSO FileIO::TextFile : public FileIO::File
{
public:
    static constexpr bool DefaultWriteNewlineAsCRLF = true;

    TextFile();
    TextFile(TextFile&& rhs) noexcept = default;

    // the text encoding determines:
    //     - when reading a file:
    //           - the encoding to use when a BOM is not present
    //           - if a BOM is present, that text encoding will be used instead
    //     - when writing a file:
    //           - whether a BOM should be written when writing UTF-8
    //     - whether text read and written (using methods in this class) should
    //       be converted to/from UTF-8 and another encoding format
    TextEncoding GetTextEncoding() const             { return m_textEncoding; }
    void SetTextEncoding(TextEncoding text_encoding) { m_textEncoding = std::move(text_encoding); }

    // sets the style of newline characters used when writing lines
    void SetWriteNewlineAsCRLF(bool write_newline_as_crlf) { m_writeNewlineAsCRLF = write_newline_as_crlf; }

    // sets the encoding and newline properties using a PropertyRetriever,
    // or an object that can create a PropertyRetriever
    template<typename T>
    void SetProperties(T&& property_retriever_or_object);

    // parses the properties of the non-null objects using a PropertyRetriever,
    // or an object that can create a PropertyRetriever
    template<typename T>
    static void ParseProperties(T&& property_retriever_or_object, TextEncoding* text_encoding, bool* write_newline_as_crlf);

    // opens an existing file for reading ("rb");
    // if a BOM exists, it is automatically read and lines from the file are read using that text encoding;
    // otherwise lines from the file are read based on the value of m_textEncoding
    TextFile& OpenForTextReading(InterfaceString file_path, int share_flag = INT_MIN);

    // creates a new file for writing ("wb");
    // a BOM is written conditionally based on the value of m_textEncoding;
    // lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF
    TextFile& OpenForTextWritingCreate(InterfaceString file_path, int share_flag = INT_MIN);

    // opens an existing file for writing ("rb+") and seeks to the end of the file;
    // lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF
    TextFile& OpenForTextWritingAppend(InterfaceString file_path, int share_flag = INT_MIN);

    // opens a file for writing, or for appending, based on the append flag;
    // lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF
    TextFile& OpenForTextWriting(InterfaceString file_path, bool append, int share_flag = INT_MIN);

    // flushes the stream and returns the current file position
    int64_t FlushAndGetPosition();

    // reads characters up to a newline character (which is processed), returning true if a line was read
    bool ReadLine(std::string& line);

    // writes the characters of a string, throwing an exception on error;
    // unlike File::Write, if writing newline characters as \r\n, the string will be parsed
    // and \r characters will be added as necessary
    template<typename T>
    TextFile& WriteString(T&& text);
    TextFile& WriteString(const char* text) { return WriteString(std::string_view(text)); }

    // writes the characters of a string, along with a newline character, throwing an exception on error
    template<typename T>
    TextFile& WriteLine(T&& text);

    // writes a blank line, throwing an exception on error
    TextFile& WriteLine();

    // writes the formatted string, throwing an exception on error
    template<typename... Args>
    TextFile& WriteFormattedString(const char* formatter, Args const&... args);

    // writes the formatted string, along with a newline character, throwing an exception on error
    template<typename... Args>
    TextFile& WriteFormattedLine(const char* formatter, Args const&... args);

protected:
    std::unique_ptr<OutputStreamBuffer> CreateOutputStream() override;

private:
    static void ParsePropertiesWorker(PropertyRetriever& property_retriever, TextEncoding* text_encoding, bool* write_newline_as_crlf);

    template<typename T>
    void WriteStringInCorrectEncoding(T&& text);

    void WriteStringEnsuringCRLF(std::string_view text_sv, size_t newline_pos);

private:
    TextEncoding m_textEncoding;
    std::unique_ptr<TextEncoding::Converter> m_textConverter;
    bool m_writeNewlineAsCRLF;
};



// --------------------------------------------------------------------------
// File inline implementations
// --------------------------------------------------------------------------

inline size_t FileIO::File::Read(void* buffer, const size_t max_length)
{
    ASSERT(IsOpen());

    return fread(buffer, 1, max_length, m_file);
}


inline size_t FileIO::File::ReadExact(void* buffer, const size_t exact_length)
{
    if( Read(buffer, exact_length) != exact_length )
        OnReadError();

    return exact_length;
}


inline FileIO::File& FileIO::File::Write(const void* const buffer, const size_t length)
{
    ASSERT(IsOpen());

    if( fwrite(buffer, 1, length, m_file) != length )
        OnWriteError();

    return *this;
}


template<typename T>
FileIO::File& FileIO::File::WriteBinary(const T& value)
{
    return Write(&value, sizeof(value));
}


inline void FileIO::File::SeekToEnd()
{
    Seek(0, SEEK_END);
}



// --------------------------------------------------------------------------
// TextFile inline implementations
// --------------------------------------------------------------------------

inline FileIO::TextFile::TextFile()
    :   m_writeNewlineAsCRLF(DefaultWriteNewlineAsCRLF)
{
}


template<typename T>
void FileIO::TextFile::SetProperties(T&& property_retriever_or_object)
{
    ParseProperties(std::forward<T>(property_retriever_or_object), &m_textEncoding, &m_writeNewlineAsCRLF);
}


template<typename T>
void FileIO::TextFile::ParseProperties(T&& property_retriever_or_object, TextEncoding* const text_encoding, bool* const write_newline_as_crlf)
{
    if constexpr(std::is_same_v<std::remove_cvref_t<T>, PropertyRetriever>)
    {
        ParsePropertiesWorker(std::forward<T>(property_retriever_or_object), text_encoding, write_newline_as_crlf);
    }

    else
    {
        const std::unique_ptr<PropertyRetriever> property_retriever = property_retriever_or_object.CreatePropertyRetriever();
        ParsePropertiesWorker(*property_retriever, text_encoding, write_newline_as_crlf);
    }
}


template<typename T>
void FileIO::TextFile::WriteStringInCorrectEncoding(T&& text)
{
    if( m_textConverter != nullptr )
    {
        File::Write(m_textConverter->FromUtf8(std::forward<T>(text)));
    }

    else
    {
        File::Write(std::forward<T>(text));
    }
}


template<typename T>
FileIO::TextFile& FileIO::TextFile::WriteString(T&& text)
{
    if( !text.empty() )
    {
        if( m_writeNewlineAsCRLF )
        {
            const size_t newline_pos = text.find('\n');

            if( newline_pos != std::remove_cvref_t<T>::npos )
            {
                WriteStringEnsuringCRLF(std::forward<T>(text), newline_pos);
                return *this;
            }
        }

        WriteStringInCorrectEncoding(std::forward<T>(text));
    }

    return *this;
}


template<typename T>
FileIO::TextFile& FileIO::TextFile::WriteLine(T&& text)
{
    WriteString(std::forward<T>(text));

    return WriteLine();
}


template<typename... Args>
FileIO::TextFile& FileIO::TextFile::WriteFormattedString(const char* const formatter, Args const&... args)
{
    return WriteString(FormatText(formatter, args...));
}


template<typename... Args>
FileIO::TextFile& FileIO::TextFile::WriteFormattedLine(const char* const formatter, Args const&... args)
{
    return WriteLine(FormatText(formatter, args...));
}
