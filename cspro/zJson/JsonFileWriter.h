#pragma once

#include <zJson/JsonStreamWriter.h>
#include <zToolsO/FileIO.h>
#include <fstream>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename WriterType>
class JsonFileWriterImpl : public JsonStreamWriterImpl<std::ostream, WriterType>, public JsonFileWriter
{
private:
    JsonFileWriterImpl(const std::string& file_path, std::unique_ptr<std::ofstream> file_stream, const JsonFormattingOptions formatting_options)
        :   JsonStreamWriterImpl<std::ostream, WriterType>(*file_stream, formatting_options),
            m_filePath(file_path),
            m_fileStream(std::move(file_stream))
    {
    }

public:
    JsonFileWriterImpl(const std::string& file_path, const JsonFormattingOptions formatting_options)
        :   JsonFileWriterImpl(file_path, FileIO::OpenOutputFileStream(file_path), formatting_options)
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
        JsonConsWriter<WriterType>::m_writer.reset();

        m_fileStream->close();
    }

    std::string GetRelativePath(const std::string& path) const override
    {
        return GetRelativePathForDisplay(m_filePath, path);
    }

private:
    std::string m_filePath;
    std::unique_ptr<std::ofstream> m_fileStream;
};


// restore the warnings
#pragma warning(pop)
