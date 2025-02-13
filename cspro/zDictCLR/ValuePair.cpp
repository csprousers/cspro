#include "Stdafx.h"
#include "ValuePair.h"


CSPro::Dictionary::ValuePair::ValuePair(const DictValuePair& dict_value_pair)
{
    m_from = gcnew System::String(dict_value_pair.GetFrom());
    m_to = gcnew System::String(dict_value_pair.GetTo());
}
