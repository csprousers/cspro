#include "Stdafx.h"
#include "DictionaryValue.h"


CSPro::Dictionary::DictionaryValue::DictionaryValue(const DictValue& dict_value)
    :   m_dictValue(dict_value)
{
}

System::String^ CSPro::Dictionary::DictionaryValue::Label::get()
{
    return gcnew System::String(m_dictValue.GetLabel());
}

array<CSPro::Dictionary::ValuePair^>^ CSPro::Dictionary::DictionaryValue::ValuePairs::get()
{
    array<CSPro::Dictionary::ValuePair^>^ value_pairs = gcnew array<ValuePair^>(m_dictValue.GetNumValuePairs());

    int i = 0;

    for( const auto& dict_value_pair : m_dictValue.GetValuePairs() )
    {
        value_pairs[i] = gcnew ValuePair(dict_value_pair);
        ++i;
    }

    return value_pairs;
}
