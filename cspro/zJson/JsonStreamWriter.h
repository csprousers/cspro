#pragma once

#include <zJson/JsonConsWriter.h>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename CharType, typename StreamType, typename WriterType>
class JsonStreamWriterImpl : public JsonConsWriter<CharType, WriterType>, public JsonStreamWriter<StreamType>
{
public:
    JsonStreamWriterImpl(StreamType& stream, JsonFormattingOptions formatting_options)
        :   JsonConsWriter<CharType, WriterType>(formatting_options),
            m_stream(stream)
    {
        JsonConsWriter<CharType, WriterType>::m_writer = std::make_unique<WriterType>(stream, GetJsonOptions<CharType>(formatting_options));
    }

    ~JsonStreamWriterImpl()
    {
        // destroy the writer so that the stream is finalized 
        // before we potentially lose access to m_stream
        JsonConsWriter<CharType, WriterType>::m_writer.reset();
    }

    StreamType& GetStream() override
    {
        return m_stream;
    }

    void Flush() override
    {
        JsonConsWriter<CharType, WriterType>::m_writer->flush();
    }

private:
    StreamType& m_stream;
};


// restore the warnings
#pragma warning(pop)
