#pragma once

#include <zToolsO/zToolsO.h>


CLASS_DECL_ZTOOLSO void dvaltochar(double dValue, csprochar* pBuf, int iLen, int iDec, bool bLeadingZeros = false, bool bExplicitDecimals = true);//PROTO

CLASS_DECL_ZTOOLSO std::wstring dvaltochar(double dValue, int iLen, int iDec, bool bLeadingZeros = false, bool bExplicitDecimals = true);//PROTO

CLASS_DECL_ZTOOLSO double chartodval(const csprochar *buf, int len, int dec);//PROTO

inline double chartodval(const std::wstring& buf, int dec)
{
    return chartodval(buf.c_str(), static_cast<int>(buf.length()), dec);
}

CLASS_DECL_ZTOOLSO bool hard_bounds(int len, int dec, double *min_bound, double *max_bound);//PROTO
