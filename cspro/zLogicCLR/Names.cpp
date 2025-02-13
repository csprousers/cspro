#include "Stdafx.h"
#include "Names.h"
#include <zLogicO/ReservedWords.h>


namespace
{
    std::wstring CreateUnreservedName(std::wstring text)
    {
        return CIMSAString::CreateUnreservedName(text,
                [&](const std::wstring& name_candidate)
                {
                    return !::Logic::ReservedWords::IsReservedWord(name_candidate);
                });
    }
}


bool CSPro::Logic::Names::IsValid(System::String^ name_)
{
    std::wstring name = ToWS(name_);

    return ( CIMSAString::IsName(name) && !::Logic::ReservedWords::IsReservedWord(name) );
}


System::String^ CSPro::Logic::Names::MakeName(System::String^ label)
{
    return gcnew System::String(CreateUnreservedName(ToWS(label)).c_str());
}
