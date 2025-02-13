#include "Stdafx.h"
#include "Versioning.h"
#include <zUtilO/Versioning.h>


double CSPro::Util::Versioning::Number::get()
{
    return CSPRO_VERSION_NUMBER;
}

System::String^ CSPro::Util::Versioning::DetailedString::get()
{
    return gcnew System::String(::Versioning::GetVersionDetailedString());
}

System::String^ CSPro::Util::Versioning::ReleaseDateString::get()
{
    return gcnew System::String(::Versioning::GetReleaseDateString());
}
