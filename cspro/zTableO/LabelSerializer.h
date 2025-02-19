#pragma once

#include <zToolsO/Encoders.h>


namespace TableLabelSerializer
{
    inline CString Create(const wstring_view text_sv)
    {
        return UTF8_TODO::GetCString(Encoders::ToEscapedString(UTF8_TODO::GetUtf8(SO::ToNewlineLF(text_sv)), false));
    }


    inline CString ParseV8(const wstring_view text_sv)
    {
        return UTF8_TODO::GetCString(SO::ToNewlineCRLF(Encoders::FromEscapedString(UTF8_TODO::GetUtf8(text_sv))));
    }


    inline CString Parse(const wstring_view text_sv, const CString& sVersion)
    {
        if( sVersion.CompareNoCase(_T("CSPro 8.0")) < 0 )
            return text_sv;

        return ParseV8(text_sv);        
    }
}
