#pragma once

#include <zJson/JsonWriter.h>
#include <zToolsO/Special.h>


// --------------------------------------------------------------------------
// JsonConsWriter (the wrapper around jsoncons)
// --------------------------------------------------------------------------

template<typename CharType, typename WriterType>
class JsonConsWriter : virtual public JsonWriter
{
protected:
    JsonConsWriter(JsonFormattingOptions formatting_options)
        :   m_allowFormattingModifications(formatting_options == JsonFormattingOptions::PrettySpacing)
    {
        // m_writer must be set by subclasses
    }

public:
    // --------------------------------------------------------------------------
    // key methods
    // --------------------------------------------------------------------------
    JsonWriter& Key(std::string_view key_sv) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            m_writer->key(key_sv);
        }

        else
        {
            m_writer->key(UTF8Convert::UTF8ToWide(key_sv));
        }

        return *this;
    }

    JsonWriter& Key(wstring_view key_sv) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            m_writer->key(UTF8Convert::WideToUTF8(key_sv));
        }

        else
        {
            m_writer->key(key_sv);
        }

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

    JsonWriter& Write(bool value) override
    {
        m_writer->bool_value(value);
        return *this;
    }

    JsonWriter& Write(int value) override
    {
        m_writer->int64_value(value);
        return *this;
    }

    JsonWriter& Write(unsigned int value) override
    {
        m_writer->uint64_value(value);
        return *this;
    }

    JsonWriter& Write(int64_t value) override
    {
        m_writer->int64_value(value);
        return *this;
    }

    JsonWriter& Write(uint64_t value) override
    {
        m_writer->uint64_value(value);
        return *this;
    }

    JsonWriter& Write(double value) override
    {
        m_writer->double_value(value);
        return *this;
    }

    JsonWriter& Write(std::string_view value_sv) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            m_writer->string_value(value_sv);
        }

        else
        {
            m_writer->string_value(UTF8Convert::UTF8ToWide(value_sv));
        }

        return *this;
    }

    JsonWriter& Write(wstring_view value_sv) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            m_writer->string_value(UTF8Convert::WideToUTF8(value_sv));
        }

        else
        {
            m_writer->string_value(value_sv);
        }

        return *this;
    }

    JsonWriter& WriteEngineValueDouble(double value) override
    {
        if( IsSpecial(value) )
        {
            return Write(wstring_view(SpecialValues::ValueToString(value)));
        }

        else
        {
            return Write(value);
        }
    }

    JsonWriter& Write(const JsonNode<char>& json_node) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            json_node.GetBasicJson().dump(*m_writer);
        }

        else
        {
            ASSERT(false);
        }

        return *this;
    }

    JsonWriter& Write(const JsonNode<wchar_t>& json_node) override
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            ASSERT(false);
        }

        else
        {
            json_node.GetBasicJson().dump(*m_writer);
        }

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
    std::unique_ptr<std::stack<const jsoncons::ModifiableOptions<CharType>*>> m_formattingOptionsStack;
};



template<typename CharType, typename WriterType>
JsonWriter::FormattingHolder JsonConsWriter<CharType, WriterType>::SetFormattingType(JsonFormattingType formatting_type)
{
    if( m_allowFormattingModifications )
    {
        if( m_formattingOptionsStack == nullptr )
            m_formattingOptionsStack = std::make_unique<std::stack<const jsoncons::ModifiableOptions<CharType>*>>();

        const jsoncons::ModifiableOptions<CharType>& modifiable_options = GetJsonModifiableOptions<CharType>(formatting_type);

        m_formattingOptionsStack->push(&modifiable_options);
        m_writer->ModifyOptions(&modifiable_options);

        return FormattingHolder(this);
    }

    return FormattingHolder(nullptr);
}


template<typename CharType, typename WriterType>
void JsonConsWriter<CharType, WriterType>::SetFormattingAction(JsonFormattingAction formatting_action)
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


template<typename CharType, typename WriterType>
void JsonConsWriter<CharType, WriterType>::RemoveTopmostFormattingType()
{
    ASSERT(m_formattingOptionsStack != nullptr && !m_formattingOptionsStack->empty());
    m_formattingOptionsStack->pop();

    m_writer->ModifyOptions(m_formattingOptionsStack->empty() ? nullptr :
                                                                m_formattingOptionsStack->top());
}
