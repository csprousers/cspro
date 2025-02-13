#pragma once

#ifndef WIN32

#include <zPlatformO/zPlatformO.h>
#include <string>


CLASS_DECL_ZPLATFORMO_IMPL std::wstring UTF8ToWideAndroid(const char* pUTF8String,int iStringLen = -1);
CLASS_DECL_ZPLATFORMO_IMPL std::string WideToUTF8Android(const wchar_t* pWideString,int iStringLen = -1);

CLASS_DECL_ZPLATFORMO_IMPL int UTF8BufferToWideBufferAndroid(const char* paBuffer,int iaLength,wchar_t* pwBuffer,int iwBufferSize);
CLASS_DECL_ZPLATFORMO_IMPL int WideBufferToUTF8BufferAndroid(const wchar_t* pwBuffer,int iwLength,char* paBuffer,int iaBufferSize);

CLASS_DECL_ZPLATFORMO_IMPL std::wstring TwoByteCharToWide(const uint16_t* text, size_t length = SIZE_MAX);

CLASS_DECL_ZPLATFORMO_IMPL int _wtoi(const wchar_t* pwStrNumbers);
CLASS_DECL_ZPLATFORMO_IMPL long _wtol(const wchar_t* pwStrNumbers);
CLASS_DECL_ZPLATFORMO_IMPL double _wtof(const wchar_t* pwStrNumbers);

#endif // !WIN32
