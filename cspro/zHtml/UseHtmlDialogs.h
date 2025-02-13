#pragma once

#include <zHtml/zHtml.h>


namespace UHD                           { ZHTML_API extern bool flag; }
inline bool UseHtmlDialogs()            { return UHD::flag; }
inline void SetUseHtmlDialogs(bool use) { UHD::flag = use; }


// temporarily here for an access token for the old CSPro JavaScript interface
namespace OldCSProJavaScriptInterface
{
    ZHTML_API const std::wstring& GetAccessToken();
}
