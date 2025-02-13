#include "Stdafx.h"
#include "Paths.h"
#include <zToolsO/Tools.h>


System::String^ CSPro::Util::Paths::MakeRelativeFilename(System::String^ relativePath, System::String^ filename)
{
    std::wstring relative_filename = GetRelativeFNameForDisplay(ToWS(relativePath), ToWS(filename));
    return gcnew System::String(relative_filename.c_str());
}


System::String^ CSPro::Util::Paths::MakeAbsoluteFilename(System::String^ relativePath, System::String^ filename)
{
    std::wstring absolute_filename = MakeFullPath(ToWS(relativePath), ToWS(filename));
    return gcnew System::String(absolute_filename.c_str());
}
