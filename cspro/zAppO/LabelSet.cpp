#include "stdafx.h"
#include "LabelSet.h"
#include "LanguageSerializerHelper.h"


LabelSet LabelSet::DefaultValue;


const CString& LabelSet::GetLabel(size_t language_index, const bool use_primary_label_if_undefined/* = true*/) const
{
    ASSERT(!m_labels.empty());

    // if the label is undefined or blank, use the primary label
    if( language_index >= m_labels.size() || m_labels[language_index].IsEmpty() )
    {
        if( !use_primary_label_if_undefined )
            return SO::Empty_CString;

        language_index = 0;
    }

    return m_labels[language_index];
}


void LabelSet::SetLabel(CString label, size_t language_index/* = SIZE_MAX*/)
{
    if( language_index == SIZE_MAX )
        language_index = m_languageIndex;

    // if adding a second label when the first label is blank, also assign this to the primary label
    if( language_index == 1 && m_labels.size() == 1 && m_labels.front().IsEmpty() )
        m_labels.front() = label;

    // add the label
    if( language_index >= m_labels.size() )
        m_labels.resize(language_index + 1);

    m_labels[language_index] = std::move(label);
}


void LabelSet::DeleteLabel(size_t language_index)
{
    if( language_index >= m_labels.size() )
        return;

    if( language_index == 0 )
    {
        // there must be at least one label
        if( m_labels.size() == 1 )
            return;

        // if deleting the first language's label when the next language's label was empty, delete the empty
        // one instead because that label was defaulting to the first label
        if( m_labels.size() >= 2 && m_labels[1].IsEmpty() )
            language_index = 1;
    }

    m_labels.erase(m_labels.begin() + language_index);
}


LabelSet LabelSet::CreateFromJson(const JsonNode& json_node)
{
    const LanguageSerializerHelper* const language_serializer_helper = json_node.GetSerializerHelper().Get<LanguageSerializerHelper>();

    std::vector<CString> labels;
    std::optional<size_t> first_defined_language_index;

    for( const JsonNode& label_object : json_node.GetArray() )
    {
        const std::string_view language_name_sv = label_object.GetOrConstruct<std::string_view>(JK::language);
        std::optional<size_t> language_index = ( language_serializer_helper != nullptr ) ? language_serializer_helper->GetLanguageIndex(language_name_sv) :
                                                                                           std::nullopt;

        if( !language_index.has_value() )
        {
            if( labels.empty() )
            {
                // only log a warning if a language was specified
                if( !language_name_sv.empty() )
                    json_node.LogWarning("The language '%s' is not valid but the label will be used as the primary label", std::string(language_name_sv).c_str());

                language_index = 0;
            }

            else
            {
                json_node.LogWarning("The language '%s' is not valid and the label will be discarded", std::string(language_name_sv).c_str());
                continue;
            }
        }

        if( *language_index >= labels.size() )
        {
            labels.resize(*language_index + 1);
        }

        else if( !labels[*language_index].IsEmpty() )
        {
            json_node.LogWarning("The language '%s' has duplicate labels and all but the first label will be discarded", std::string(language_name_sv).c_str());
        }

        labels[*language_index] = label_object.GetOrConstruct<CString>(JK::text);

        if( !first_defined_language_index.has_value() )
            first_defined_language_index = *language_index;
    }

    // make sure there is at least one label
    if( labels.empty() )
    {
        json_node.LogWarning("No label was defined");
        labels.emplace_back();
    }

    // make sure the primary label is set to a value (from the first defined language)
    else if( labels.size() > 1 && labels.front().IsEmpty() )
    {
        ASSERT(first_defined_language_index.has_value());
        labels.front() = labels[*first_defined_language_index];
    }

    return LabelSet(std::move(labels));
}


void LabelSet::WriteJson(JsonWriter& json_writer) const
{
    const LanguageSerializerHelper* const language_serializer_helper = json_writer.GetSerializerHelper().Get<LanguageSerializerHelper>();

    size_t labels_to_serialize = m_labels.size();
    bool serialize_languages;

    if( language_serializer_helper != nullptr )
    {
        labels_to_serialize = std::min(labels_to_serialize, language_serializer_helper->GetNumLanguages());

        // only serialize the language name when more than one label exists (or in verbose mode)
        serialize_languages = ( json_writer.Verbose() || labels_to_serialize > 1 );
    }

    else
    {
        serialize_languages = false;
    }

    ASSERT(labels_to_serialize > 0);

    std::optional<JsonWriter::FormattingHolder> tight_formatting_holder;

    if( labels_to_serialize == 1 && !serialize_languages )
        tight_formatting_holder.emplace(json_writer.SetFormattingType(JsonFormattingType::Tight));

    json_writer.BeginArray();

    for( size_t i = 0; i < labels_to_serialize; ++i )
    {
        const CString& label = m_labels[i];

        if( i == 0 || ( !label.IsEmpty() && label != m_labels.front() ) )
        {
            json_writer.BeginObject();

            json_writer.Write(JK::text, m_labels[i]);

            if( serialize_languages )
                json_writer.Write(JK::language, language_serializer_helper->GetLanguageName(i));

            json_writer.EndObject();
        }
    }

    json_writer.EndArray();
}


void LabelSet::serialize(Serializer& ar)
{
    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_labels;
    }

    else
    {
        m_labels = Pre80SerializableLabelToLabels(ar.Read<std::string>());
    }
}


std::vector<CString> LabelSet::Pre80SerializableLabelToLabels(const std::string_view serializable_label_sv)
{
    constexpr char LabelSeparator = '|';

    LabelSet label_set;
    size_t language_index = 0;

    for( std::string label : SO::SplitString(serializable_label_sv, LabelSeparator) )
        label_set.SetLabel(UTF8_TODO::GetCString(std::move(label)), language_index++);

    return label_set.m_labels;
}
