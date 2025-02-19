#pragma once

#ifndef WIN32

#include <zToolsO/zToolsO.h>
#include <string>


CLASS_DECL_ZTOOLSO std::wstring UTF8ToWideAndroid(const char* pUTF8String,int iStringLen = -1);
CLASS_DECL_ZTOOLSO std::string WideToUTF8Android(const wchar_t* pWideString,int iStringLen = -1);

CLASS_DECL_ZTOOLSO int UTF8BufferToWideBufferAndroid(const char* paBuffer,int iaLength,wchar_t* pwBuffer,int iwBufferSize);
CLASS_DECL_ZTOOLSO int WideBufferToUTF8BufferAndroid(const wchar_t* pwBuffer,int iwLength,char* paBuffer,int iaBufferSize);

CLASS_DECL_ZTOOLSO std::wstring TwoByteCharToWide(const uint16_t* text, size_t length = SIZE_MAX);

CLASS_DECL_ZTOOLSO int _wtoi(const wchar_t* pwStrNumbers);
CLASS_DECL_ZTOOLSO long _wtol(const wchar_t* pwStrNumbers);
CLASS_DECL_ZTOOLSO double _wtof(const wchar_t* pwStrNumbers);

#endif // !WIN32
