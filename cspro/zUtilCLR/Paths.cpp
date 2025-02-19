#include "Stdafx.h"
#include "Paths.h"
#include <zToolsO/Tools.h>


System::String^ CSPro::Util::Paths::MakeRelativeFilename(System::String^ relativePath, System::String^ filename)
{
    const std::string relative_path = GetRelativePathForDisplay(clr_helpers::to_string(relativePath), clr_helpers::to_string(filename));
    return clr_helpers::to_SystemString(relative_path);
}


System::String^ CSPro::Util::Paths::MakeAbsoluteFilename(System::String^ relativePath, System::String^ filename)
{
    std::wstring absolute_filename = MakeFullPath(clr_helpers::to_wstring(relativePath), clr_helpers::to_wstring(filename));
    return gcnew System::String(absolute_filename.c_str());
}
