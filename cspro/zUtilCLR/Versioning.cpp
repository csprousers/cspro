#include "Stdafx.h"
#include "Versioning.h"
#include <zUtilO/Versioning.h>


double CSPro::Util::Versioning::Number::get()
{
    return ::Versioning::Number;
}


System::String^ CSPro::Util::Versioning::DetailedString::get()
{
    return clr_helpers::to_SystemString(::Versioning::GetVersionDetailedString());
}


System::String^ CSPro::Util::Versioning::ReleaseDateString::get()
{
    return clr_helpers::to_SystemString(::Versioning::GetReleaseDateString());
}
