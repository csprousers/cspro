#pragma once

#include <zJson/JsonConsWriter.h>

// disable warnings related to using multiple inheritance
#pragma warning(push)
#pragma warning(disable:4250) // 'class1': inherits 'class2::member' via dominance


template<typename StreamType, typename WriterType>
class JsonStreamWriterImpl : public JsonConsWriter<WriterType>, public JsonStreamWriter<StreamType>
{
public:
    JsonStreamWriterImpl(StreamType& stream, const JsonFormattingOptions formatting_options)
        :   JsonConsWriter<WriterType>(formatting_options),
            m_stream(stream)
    {
        JsonConsWriter<WriterType>::m_writer = std::make_unique<WriterType>(stream, GetJsonOptions(formatting_options));
    }

    ~JsonStreamWriterImpl()
    {
        // destroy the writer so that the stream is finalized
        // before we potentially lose access to m_stream
        JsonConsWriter<WriterType>::m_writer.reset();
    }

    StreamType& GetStream() override
    {
        return m_stream;
    }

    void Flush() override
    {
        JsonConsWriter<WriterType>::m_writer->flush();
    }

private:
    StreamType& m_stream;
};


// restore the warnings
#pragma warning(pop)
