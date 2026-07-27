#include "Stdafx.h"
#include "ValuePair.h"


CSPro::Dictionary::ValuePair::ValuePair(const DictValuePair& dict_value_pair)
{
    m_from = clr_helpers::to_SystemString(dict_value_pair.GetFrom());
    m_to = clr_helpers::to_SystemString(dict_value_pair.GetTo());
}
