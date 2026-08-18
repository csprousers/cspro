#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/PropertyRetriever.h>
#include <zToolsO/TextEncoding.h>
#include <sstream>

namespace FileIO { class File; }


// --------------------------------------------------------------------------
// FileIO::File
//
// The FileIO::File class and its subclass FileIO::TextFile wrap FILE
// functionality. All files are opened as binary, although FileIO::TextFile
// can properly encode text after reading or before writing.
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO FileIO::File
{
public:
    File();
    File(const File& rhs) = delete;
    File(File&& rhs) noexcept;

    virtual ~File();

    File& operator=(const File& rhs) = delete;
    File& operator=(File&& rhs) = delete;

    // If set to true, the file is automatically deleted:
    //   - if the file has not been closed manually prior to destruction
    //   - on write errors
    File& SetDeleteFileOnError(bool delete_file) noexcept { m_deleteFileOnError = delete_file; return *this; }

    // Indicates if a file is open.
    bool IsOpen() const noexcept { return ( m_file != nullptr ); }

    // Returns the path of the open file.
    const std::string& GetPath() const noexcept { return m_filePath; }

    // Opens a file, throwing an exception on error.
    File& Open(InterfaceString file_path, const char* mode, int share_flag = INT_MIN);

    // Opens an existing file for reading ("rb").
    File& OpenForReading(InterfaceString file_path, int share_flag = INT_MIN);

    // Creates a new file for writing ("wb").
    File& OpenForWritingCreate(InterfaceString file_path, int share_flag = INT_MIN);

    // Closes a file, throwing an exception on error.
    void Close() { Close(true); }

    // Returns the current file position.
    int64_t GetPosition();

    // Reads bytes up to the length specified, returning the number of bytes read.
    size_t Read(void* buffer, size_t max_length);

    // Reads bytes to the exact length specified, throwing an exception on error.
    size_t ReadExact(void* buffer, size_t exact_length);

    // Writes bytes, throwing an exception on error.
    File& Write(const void* buffer, size_t length);

    // Writes the characters of a string, throwing an exception on error.
    // Note that \n characters are written simply as \n. To potentially write
    // such characters as \r\n, use the TextFile class and its WriteString method.
    File& Write(const std::string& text)  { return Write(text.data(), text.length()); }
    File& Write(std::string_view text_sv) { return Write(text_sv.data(), text_sv.length()); }
    File& Write(const char* text)         { return Write(text, strlen(text)); }

    // Writes the binary representation of a data type.
    template<typename T>
    File& WriteBinary(const T& value);

    // Seeks to a different position of the file, throwing an exception on error.
    void Seek(int64_t offset, int origin);
    void SeekToEnd();

    // Returns a std::ostream that can be used for outputting data.
    // Only outputs are implemented so do not use this stream for random access.
    std::unique_ptr<std::ostream> GetOutputStream();

protected:
    class OutputStreamBuffer;
    class OutputStreamBufferWithCRLFHandling;
    virtual bool OutputStreamRequiresCRLFHandling() const;

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



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline size_t FileIO::File::Read(void* const buffer, const size_t max_length)
{
    ASSERT(IsOpen());

    return fread(buffer, 1, max_length, m_file);
}


inline size_t FileIO::File::ReadExact(void* const buffer, const size_t exact_length)
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
