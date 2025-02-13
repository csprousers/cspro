#pragma once

#ifdef _MFC_VER

#error You should not include this file for platforms that have MFC

#else

#ifndef WIN_DESKTOP
#include <zPlatformO/PortableWindowsDefines.h>
#endif

#include <zPlatformO/PortableMFCDefines.h>

// CObject
#include <zPlatformO/CSProObject.h>
using CObject = CCSProObject;

// CString
#include <zPlatformO/stdstring.h>
using CString = CStdString;
using CStringA = CStdStringA;
using CStringW = CStdStringW;

// CSize + CPoint + CSize
#include <zPlatformO/CSProSize.h>
using CSize = CCSProSize;
#include <zPlatformO/CSProPoint.h>
using CPoint = CCSProPoint;
#include <zPlatformO/CSProRect.h>
using CRect = CCSProRect;

// CFile + CStdioFile
#include <zPlatformO/CSProStdioFile.h>
using CFile = CCSProFile;
using CStdioFile = CCSProStdioFile;


#ifndef GetRValue
#define GetRValue(rgb)      (((uint32_t) (rgb)) & 0xff)
#define GetGValue(rgb)      ((((uint32_t) (rgb)) >> 8) & 0xff)
#define GetBValue(rgb)      ((((uint32_t) (rgb)) >> 16) & 0xff)
#endif

#endif // _MFC_VER
