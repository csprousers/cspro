#pragma once

inline CString forceEndWithSlash(CString url)
{
    if (url.IsEmpty() || url[url.GetLength() - 1] != _T('/'))
        url += _T('/');
    return url;
}
