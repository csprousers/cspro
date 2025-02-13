#pragma once

#include <zToolsO/Encoders.h>


namespace TableLabelSerializer
{
    inline std::wstring Create(wstring_view text_sv)
    {
        return Encoders::ToEscapedString(SO::ToNewlineLF(text_sv), false);
    }


    inline CString ParseV8(wstring_view text_sv)
    {
        return WS2CS(SO::ToNewlineCRLF(Encoders::FromEscapedString(text_sv)));
    }


    inline CString Parse(wstring_view text_sv, const CString& sVersion)
    {
        if( sVersion.CompareNoCase(_T("CSPro 8.0")) < 0 )
            return text_sv;

        return ParseV8(text_sv);        
    }
}
