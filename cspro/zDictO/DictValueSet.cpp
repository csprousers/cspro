#include "StdAfx.h"
#include "DictValueSet.h"


DictValueSet::DictValueSet()
    :   m_symbolIndex(-1)
{
}


void DictValueSet::SetValues(std::vector<DictValue> dict_values)
{
    m_dictValues = std::move(dict_values);
}


void DictValueSet::AddValue(DictValue dict_value)
{
    m_dictValues.emplace_back(std::move(dict_value));
}


void DictValueSet::InsertValue(size_t index, DictValue dict_value)
{
    ASSERT(index <= m_dictValues.size());
    m_dictValues.insert(m_dictValues.begin() + index, std::move(dict_value));
}


void DictValueSet::RemoveValue(size_t index)
{
    ASSERT(index < m_dictValues.size());
    m_dictValues.erase(m_dictValues.begin() + index);
}


void DictValueSet::RemoveAllValues()
{
    m_dictValues.clear();
}


void DictValueSet::LinkValueSet(DictValueSet& source_dict_value_set)
{
    // use a UUID as the linking identifier if one has not already been set
    if( source_dict_value_set.m_linkedValueSetCode.empty() )
        source_dict_value_set.m_linkedValueSetCode = CreateUuid();

    m_linkedValueSetCode = source_dict_value_set.m_linkedValueSetCode;
}


void DictValueSet::LinkValueSetByCode(std::wstring code)
{
    m_linkedValueSetCode = std::move(code);
}


void DictValueSet::UnlinkValueSet()
{
    m_linkedValueSetCode.clear();
}


size_t DictValueSet::GetNumToValues() const
{
    size_t num_to_values = 0;

    for( const DictValue& dict_value : m_dictValues )
        num_to_values += dict_value.GetNumToValues();

    return num_to_values;
}


std::tuple<double, double> DictValueSet::GetMinMax() const
{
    std::tuple<double, double> min_max(std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::lowest());

    for( const DictValue& dict_value : m_dictValues )
    {
        if( dict_value.IsSpecial() )
            continue;

        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
        {
            auto process = [&](const CString& text)
            {
                if( !SO::IsBlank(text) )
                {
                    double value = CIMSAString::fVal(text);
                    std::get<0>(min_max) = std::min(value, std::get<0>(min_max));
                    std::get<1>(min_max) = std::max(value, std::get<1>(min_max));
                }
            };

            process(dict_value_pair.GetFrom());
            process(dict_value_pair.GetTo());
        }
    }

    return min_max;
}


DictValueSet DictValueSet::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    DictValueSet dict_value_set;

    auto dict_serializer_helper = json_node.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentValueSet(dict_value_set);

    dict_value_set.DictNamedBase::ParseJsonInput(json_node);

    dict_value_set.m_linkedValueSetCode = json_node.GetOrDefault(JK::link, SO::EmptyString);

    dict_value_set.m_dictValues = json_node.GetArrayOrEmpty(JK::values).GetVector<DictValue>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A value was not added to '%s' due to errors: %s"),
                                 dict_value_set.GetName().GetString(),
                                 exception.GetErrorMessage().c_str());
        });

    return dict_value_set;
}


void DictValueSet::WriteJson(JsonWriter& json_writer) const
{
    auto dict_serializer_helper = json_writer.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentValueSet(*this);

    json_writer.BeginObject();

    DictNamedBase::WriteJson(json_writer);

    // only write the values for the first encountered value set
    bool write_values = true;

    if( IsLinkedValueSet() )
    {
        bool is_really_linked_value_set = true;

        if( dict_serializer_helper != nullptr )
        {
            // if the values have already been written, there is no reason to write them again
            if( dict_serializer_helper->HasValueSetBeenSerialized(m_linkedValueSetCode) )
            {
                write_values = false;
            }

            // if this is the first time encountering this linked value set, confirm that it still is
            // actually linked to other value sets
            else if( dict_serializer_helper->GetDictionary().CountValueSetLinks(*this) >= 2 )
            {
                dict_serializer_helper->MarkValueSetAsSerialized(m_linkedValueSetCode);
            }

            else
            {
                is_really_linked_value_set = false;
            }
        }

        if( is_really_linked_value_set )
            json_writer.Write(JK::link, m_linkedValueSetCode);
    }

    if( json_writer.Verbose() || write_values )
        json_writer.Write(JK::values, m_dictValues);

    json_writer.EndObject();
}


void DictValueSet::serialize(Serializer& ar)
{
    DictNamedBase::serialize(ar);

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
        SetNote(ar.Read<CString>());

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_error

    ar & m_symbolIndex;

    // when using linked value sets, only serialize the values for the first encountered value set
    bool serialize_values = true;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_6_000_1) )
    {
        if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        {
            ar & m_linkedValueSetCode;
        }

        else
        {
            int64_t int64_t_code;
            ar.Dump(&int64_t_code, sizeof(int64_t_code));

            if( int64_t_code != 0 )
                m_linkedValueSetCode = IntToString(int64_t_code);
        }

        if( ar.IsSaving() && IsLinkedValueSet() )
        {
            auto dict_serializer_helper = ar.GetSerializerHelper().Get<DictionarySerializerHelper>();
            ASSERT(dict_serializer_helper!= nullptr);

            if( dict_serializer_helper->HasValueSetBeenSerialized(m_linkedValueSetCode) )
            {
                serialize_values = false;                
            }

            else
            {
                dict_serializer_helper->MarkValueSetAsSerialized(m_linkedValueSetCode);
            }
        }

        ar & serialize_values;
    }

    if( serialize_values )
    {
        ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumValues
        ar & m_dictValues;    
    }
}
