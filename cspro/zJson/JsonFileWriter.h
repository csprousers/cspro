#pragma once

#include <zJson/JsonStreamWriter.h>
#include <zToolsO/FileIO.h>
#include <fstream>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename WriterType>
class JsonFileWriterImpl : public JsonStreamWriterImpl<char, std::ostream, WriterType>, public JsonFileWriter
{
private:
    JsonFileWriterImpl(std::wstring filename, std::unique_ptr<std::ofstream> file_stream, JsonFormattingOptions formatting_options)
        :   JsonStreamWriterImpl<char, std::ostream, WriterType>(*file_stream, formatting_options),
            m_filename(std::move(filename)),
            m_fileStream(std::move(file_stream))
    {
    }

public:
    JsonFileWriterImpl(NullTerminatedString filename, JsonFormattingOptions formatting_options)
        :   JsonFileWriterImpl(filename, FileIO::OpenOutputFileStream(filename), formatting_options)
    {
    }

    ~JsonFileWriterImpl()
    {
        if( m_fileStream->is_open() )
            Close();
    }

    void Close() override
    {
        ASSERT(m_fileStream->is_open());

        // destroy the writer so that the stream is finalized before the file is closed
        JsonConsWriter<char, WriterType>::m_writer.reset();

        m_fileStream->close();
    }

    std::wstring GetRelativePath(const std::wstring& path) const override
    {
        return GetRelativeFNameForDisplay(m_filename, path);
    }

private:
    std::wstring m_filename;
    std::unique_ptr<std::ofstream> m_fileStream;
};


// restore the warnings
#pragma warning(pop)
