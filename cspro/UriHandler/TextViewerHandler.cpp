#include "stdafx.h"


namespace
{
    constexpr wstring_view TextViewerPropertyFilename = _T("file");
}


void HandleTextViewerUri(const std::map<std::wstring, std::wstring>& properties)
{
    const auto& filename_lookup = properties.find(TextViewerPropertyFilename);

    if( filename_lookup == properties.cend() )
    {
        throw CSProException(_T("The CSPro URI that launches Text Viewer must contain the property: %s"),
                             std::wstring(TextViewerPropertyFilename).c_str());
    }

    ViewFileInTextViewer(filename_lookup->second);
}
