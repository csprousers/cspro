#include "Stdafx.h"
#include "Names.h"
#include <zLogicO/ReservedWords.h>


bool CSPro::Logic::Names::IsValid(System::String^ name_)
{
    const std::string name = clr_helpers::to_string(name_);

    return ( CIMSAString::IsName(name) && !::Logic::ReservedWords::IsReservedWord(name) );
}


System::String^ CSPro::Logic::Names::MakeName(System::String^ label_)
{
    const std::string label = clr_helpers::to_string(label_);

    const std::string unreserved_name = CIMSAString::CreateUnreservedName(label,
            [&](const std::string& name_candidate)
            {
                return !::Logic::ReservedWords::IsReservedWord(name_candidate);
            });

    return clr_helpers::to_SystemString(unreserved_name);
}
