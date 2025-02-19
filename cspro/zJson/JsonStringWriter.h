#pragma once

#include <zJson/JsonConsWriter.h>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename WriterType>
class JsonStringWriterImpl : public JsonConsWriter<WriterType>, public JsonStringWriter
{
public:
    JsonStringWriterImpl(std::string& text, const JsonFormattingOptions formatting_options)
        :   JsonConsWriter<WriterType>(formatting_options),
            m_text(text)
    {
        JsonConsWriter<WriterType>::m_writer = std::make_unique<WriterType>(text, GetJsonOptions(formatting_options));
    }

    const std::string& GetString() const override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
        return m_text;
    }

    std::string ReleaseString() override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
        return ReturnProgrammingError(m_text);
    }

    SharableString ReleaseSharableString() override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
        return ReturnProgrammingError(m_text);
    }

private:
    std::string& m_text;
};



template<typename WriterType>
class JsonStringWriterOwningTextBufferImpl : public JsonStringWriterImpl<WriterType>
{
private:
    JsonStringWriterOwningTextBufferImpl(std::unique_ptr<std::string> text, const JsonFormattingOptions formatting_options)
        :   JsonStringWriterImpl<WriterType>(*text, formatting_options),
            m_ownedText(std::move(text))
    {
    }

public:
    JsonStringWriterOwningTextBufferImpl(const JsonFormattingOptions formatting_options)
        :   JsonStringWriterOwningTextBufferImpl(std::make_unique<std::string>(), formatting_options)
    {
    }

    std::string ReleaseString() override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
        return std::move(*m_ownedText);
    }

    SharableString ReleaseSharableString() override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
        return std::move(m_ownedText);
    }

private:
    std::unique_ptr<std::string> m_ownedText;
};



template<typename WriterType>
class JsonStringWriterOwningTextBufferWithRelativePathsImpl : public JsonStringWriterOwningTextBufferImpl<WriterType>
{
public:
    JsonStringWriterOwningTextBufferWithRelativePathsImpl(std::string file_path, const JsonFormattingOptions formatting_options)
        :   JsonStringWriterOwningTextBufferImpl<WriterType>(formatting_options),
            m_filePathForRelativePaths(std::move(file_path))
    {
    }

    std::string GetRelativePath(const std::string& path) const override
    {
        return GetRelativePathForDisplay(m_filePathForRelativePaths, path);
    }

private:
    std::string m_filePathForRelativePaths;
};


// restore the warnings
#pragma warning(pop)
