#pragma once

#include <zJson/JsonConsWriter.h>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename CharType, typename WriterType>
class JsonStringWriterImpl : public JsonConsWriter<CharType, WriterType>, public JsonStringWriter<CharType>
{
public:
    JsonStringWriterImpl(std::basic_string<CharType>& text, JsonFormattingOptions formatting_options)
        :   JsonConsWriter<CharType, WriterType>(formatting_options),
            m_text(text)
    {
        JsonConsWriter<CharType, WriterType>::m_writer = std::make_unique<WriterType>(text, GetJsonOptions<CharType>(formatting_options));
    }

    const std::basic_string<CharType>& GetString() override
    {
        JsonConsWriter<CharType, WriterType>::m_writer->flush();
        return m_text;
    }

private:
    std::basic_string<CharType>& m_text;
};



template<typename CharType, typename WriterType>
class JsonStringWriterOwningTextBufferImpl : public JsonStringWriterImpl<CharType, WriterType>
{
private:
    JsonStringWriterOwningTextBufferImpl(std::unique_ptr<std::basic_string<CharType>> text, JsonFormattingOptions formatting_options)
        :   JsonStringWriterImpl<CharType, WriterType>(*text, formatting_options),
            m_ownedText(std::move(text))
    {
    }

public:
    JsonStringWriterOwningTextBufferImpl(JsonFormattingOptions formatting_options)
        :   JsonStringWriterOwningTextBufferImpl(std::make_unique<std::basic_string<CharType>>(), formatting_options)
    {
    }

private:
    std::unique_ptr<std::basic_string<CharType>> m_ownedText;
};



template<typename WriterType>
class JsonStringWriterOwningTextBufferWithRelativePathsImpl : public JsonStringWriterOwningTextBufferImpl<wchar_t, WriterType>
{
public:
    JsonStringWriterOwningTextBufferWithRelativePathsImpl(std::wstring filename, JsonFormattingOptions formatting_options)
        :   JsonStringWriterOwningTextBufferImpl<wchar_t, WriterType>(formatting_options),
            m_filenameForRelativePaths(std::move(filename))
    {
    }

    std::wstring GetRelativePath(const std::wstring& path) const override
    {
        return GetRelativeFNameForDisplay(m_filenameForRelativePaths, path);
    }

private:
    std::wstring m_filenameForRelativePaths;
};


// restore the warnings
#pragma warning(pop)
