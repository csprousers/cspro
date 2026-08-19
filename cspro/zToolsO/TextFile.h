#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/File.h>

namespace FileIO { class File; class TextFile; }


// --------------------------------------------------------------------------
// FileIO::TextFile
//
// The FileIO::TextFile class, a subclass of FileIO::File, wraps FILE
// functionality. All files are opened as binary, but FileIO::TextFile can:
//   - convert UTF-8 text to other encoding formats
//   - process a BOM
//   - write newlines as either \n or \r\n
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO FileIO::TextFile : public FileIO::File
{
public:
    static constexpr bool DefaultWriteNewlineAsCRLF = true;

    TextFile();
    TextFile(TextFile&& rhs) noexcept = default;

    // The text encoding determines:
    //   - When reading a file:
    //     - the encoding to use when a BOM is not present
    //     - if a BOM is present, that text encoding will be used instead
    //   - When writing a file:
    //     - whether a BOM should be written when writing UTF-8
    //     - whether text read and written (using methods in this class) should
    //       be converted to/from UTF-8 and another encoding format
    TextEncoding GetTextEncoding() const noexcept             { return m_textEncoding; }
    void SetTextEncoding(TextEncoding text_encoding) noexcept { m_textEncoding = std::move(text_encoding); }

    // Sets the style of newline characters used when writing lines.
    void SetWriteNewlineAsCRLF(bool write_newline_as_crlf) noexcept { m_writeNewlineAsCRLF = write_newline_as_crlf; }

    // Sets the encoding and newline properties using a PropertyRetriever,
    // or an object that can create a PropertyRetriever.
    template<typename T>
    void SetProperties(T&& property_retriever_or_object);

    // Parses the properties of the non-null objects using a PropertyRetriever,
    // or an object that can create a PropertyRetriever.
    template<typename T>
    static void ParseProperties(T&& property_retriever_or_object, TextEncoding* text_encoding, bool* write_newline_as_crlf);

    // Opens an existing file for reading ("rb").
    // If a BOM exists, it is automatically read and lines from the file are read using that text encoding.
    // Otherwise, lines from the file are read based on the value of m_textEncoding
    TextFile& OpenForTextReading(InterfaceString file_path, int share_flag = INT_MIN);

    // Creates a new file for writing ("wb").
    // A BOM is written conditionally based on the value of m_textEncoding.
    // Lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF.
    TextFile& OpenForTextWritingCreate(InterfaceString file_path, int share_flag = INT_MIN);

    // Opens an existing file for writing ("rb+") and seeks to the end of the file.
    // Lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF.
    TextFile& OpenForTextWritingAppend(InterfaceString file_path, int share_flag = INT_MIN);

    // Opens a file for writing, or for appending, based on the append flag.
    // Lines are written to the file based on the values of m_textEncoding and m_writeNewlineAsCRLF.
    TextFile& OpenForTextWriting(InterfaceString file_path, bool append, int share_flag = INT_MIN);

    // Opens a file for reading and writing ("rb+" or "wb+" if create is true) and potentially seeks to the end of the file.
    // When opening files in this mode, you must manually call Flush everytime you switch between reading and writing.
    TextFile& OpenForTextReadingAndWriting(InterfaceString file_path, bool create, bool append, int share_flag = INT_MIN);

    // Reads characters up to a newline character (which is processed),
    // returning true if a line was read.
    bool ReadLine(std::string& line);

    // Writes the characters of a string, throwing an exception on error.
    // Unlike File::Write, if writing newline characters as \r\n, the string
    // will be parsed and \r characters will be added as necessary.
    template<typename T>
    TextFile& WriteString(T&& text);
    TextFile& WriteString(const char* text) { return WriteString(std::string_view(text)); }

    // Writes the characters of a string, along with a newline character,
    // throwing an exception on error.
    template<typename T>
    TextFile& WriteLine(T&& text);

    // Writes a blank line, throwing an exception on error.
    TextFile& WriteLine();

    // Writes the formatted string, throwing an exception on error.
    template<typename... Args>
    TextFile& WriteFormattedString(const char* formatter, Args const&... args);

    // Writes the formatted string, along with a newline character,
    // throwing an exception on error.
    template<typename... Args>
    TextFile& WriteFormattedLine(const char* formatter, Args const&... args);

protected:
    bool OutputStreamRequiresCRLFHandling() const override;

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
// inline implementations
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
