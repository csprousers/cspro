#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/FileIO.h>
#include <istream>


// Utf8InputFileStream is a stream that wraps a UTF-8 file, whether or not it contains a BOM,
// and returns wide characters; access to the stream throws FileIO::Exception exceptions

class CLASS_DECL_ZTOOLSO Utf8InputFileStream : public std::wistream
{
private:
    Utf8InputFileStream(std::unique_ptr<std::wstreambuf> stream_buffer);

public:
    // creates a stream based on a file that is already open; control of the file will be taken over by this object
    Utf8InputFileStream(std::wstring filename, FileIO::FileAndSize file_and_size);

    // opens a file and creates a stream from it; open errors result in FileIO::Exception exceptions
    Utf8InputFileStream(NullTerminatedString filename)
        :   Utf8InputFileStream(filename, FileIO::OpenFile(filename))
    {
    }

    int GetPercentRead() const;

private:
    std::unique_ptr<std::wstreambuf> m_streamBuffer;
};
