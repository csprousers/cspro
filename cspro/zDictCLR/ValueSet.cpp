#include "Stdafx.h"
#include "ValueSet.h"
#include <zDictO/DDClass.h>


CSPro::Dictionary::ValueSet::ValueSet(const DictValueSet& dict_value_set)
    :   m_dictValueSet(dict_value_set)
{
}

System::String^ CSPro::Dictionary::ValueSet::Name::get()
{
    return gcnew System::String(m_dictValueSet.GetName());
}

System::String^ CSPro::Dictionary::ValueSet::Label::get()
{
    return gcnew System::String(m_dictValueSet.GetLabel());
}

array<CSPro::Dictionary::DictionaryValue^>^ CSPro::Dictionary::ValueSet::Values::get()
{
    array<DictionaryValue^>^ values = gcnew array<DictionaryValue^>(m_dictValueSet.GetNumValues());

    int i = 0;

    for( const auto& dict_value : m_dictValueSet.GetValues() )
    {
        values[i] = gcnew DictionaryValue(dict_value);
        ++i;
    }

    return values;
}
