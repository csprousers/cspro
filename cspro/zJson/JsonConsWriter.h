#pragma once

#include <zJson/JsonWriter.h>


// --------------------------------------------------------------------------
// JsonConsWriter (the wrapper around jsoncons)
// --------------------------------------------------------------------------

template<typename WriterType>
class JsonConsWriter : virtual public JsonWriter
{
protected:
    JsonConsWriter(const JsonFormattingOptions formatting_options)
        :   m_allowFormattingModifications(formatting_options == JsonFormattingOptions::PrettySpacing)
    {
        // m_writer must be set by subclasses
    }

public:
    // --------------------------------------------------------------------------
    // key methods
    // --------------------------------------------------------------------------
    JsonWriter& Key(const std::string_view key_sv) override
    {
        m_writer->key(key_sv);
        return *this;
    }

    // --------------------------------------------------------------------------
    // object methods
    // --------------------------------------------------------------------------
    JsonWriter& BeginObject() override
    {
        m_writer->begin_object();
        return *this;
    }

    JsonWriter& EndObject() override
    {
        m_writer->end_object();
        return *this;
    }


    // --------------------------------------------------------------------------
    // array methods
    // --------------------------------------------------------------------------
    JsonWriter& BeginArray() override
    {
        m_writer->begin_array();
        return *this;
    }

    JsonWriter& EndArray() override
    {
        m_writer->end_array();
        return *this;
    }


    // --------------------------------------------------------------------------
    // writing methods
    // --------------------------------------------------------------------------
    JsonWriter& WriteNull() override
    {
        m_writer->null_value();
        return *this;
    }

    JsonWriter& Write(const bool value) override
    {
        m_writer->bool_value(value);
        return *this;
    }

    JsonWriter& Write(const int value) override
    {
        m_writer->int64_value(value);
        return *this;
    }

    JsonWriter& Write(const unsigned int value) override
    {
        m_writer->uint64_value(value);
        return *this;
    }

    JsonWriter& Write(const int64_t value) override
    {
        m_writer->int64_value(value);
        return *this;
    }

    JsonWriter& Write(const uint64_t value) override
    {
        m_writer->uint64_value(value);
        return *this;
    }

    JsonWriter& Write(const double value) override
    {
        m_writer->double_value(value);
        return *this;
    }

    JsonWriter& Write(const std::string_view value_sv) override
    {
        m_writer->string_value(value_sv);
        return *this;
    }

    JsonWriter& Write(const JsonNode& json_node) override
    {
        json_node.GetBasicJson().dump(*m_writer);
        return *this;
    }


    // --------------------------------------------------------------------------
    // formatting methods
    // --------------------------------------------------------------------------
    FormattingHolder SetFormattingType(JsonFormattingType formatting_type) override;
    void SetFormattingAction(JsonFormattingAction formatting_action) override;

private:
    void RemoveTopmostFormattingType() override;


protected:
    std::unique_ptr<WriterType> m_writer;

private:
    const bool m_allowFormattingModifications;
    std::unique_ptr<std::stack<const jsoncons::ModifiableOptions<char>*>> m_formattingOptionsStack;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename WriterType>
JsonWriter::FormattingHolder JsonConsWriter<WriterType>::SetFormattingType(const JsonFormattingType formatting_type)
{
    if( m_allowFormattingModifications )
    {
        if( m_formattingOptionsStack == nullptr )
            m_formattingOptionsStack = std::make_unique<std::stack<const jsoncons::ModifiableOptions<char>*>>();

        const jsoncons::ModifiableOptions<char>& modifiable_options = GetJsonModifiableOptions(formatting_type);

        m_formattingOptionsStack->push(&modifiable_options);
        m_writer->ModifyOptions(&modifiable_options);

        return FormattingHolder(this);
    }

    return FormattingHolder(nullptr);
}


template<typename WriterType>
void JsonConsWriter<WriterType>::SetFormattingAction(const JsonFormattingAction formatting_action)
{
    if( m_allowFormattingModifications )
    {
        switch( formatting_action )
        {
            case JsonFormattingAction::TopmostObjectLineSplitSameLine:
                m_writer->ModifyOptionsTopmostObjectLineSplits(jsoncons::line_split_kind::same_line);
                break;

            case JsonFormattingAction::TopmostObjectLineSplitMultiLine:
                m_writer->ModifyOptionsTopmostObjectLineSplits(jsoncons::line_split_kind::multi_line);
                break;
        }
    }
}


template<typename WriterType>
void JsonConsWriter<WriterType>::RemoveTopmostFormattingType()
{
    ASSERT(m_formattingOptionsStack != nullptr && !m_formattingOptionsStack->empty());
    m_formattingOptionsStack->pop();

    m_writer->ModifyOptions(m_formattingOptionsStack->empty() ? nullptr :
                                                                m_formattingOptionsStack->top());
}
