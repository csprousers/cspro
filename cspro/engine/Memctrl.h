#pragma once

#include <zToolsO/zToolsO.h>

#ifndef WIN32
#define calloc( a, b ) icalloc2( 0, 0, a, b )//Savy for unicode
#define malloc( a )    imalloc2( 0, 0, a )//Savy for unicode
#define free( a )      ifree2(0, 0, a ) //Savy for unicode
#else
#define calloc( a, b ) icalloc( _T(__FILE__), __LINE__, a, b )//Savy for unicode
#define malloc( a )    imalloc( _T(__FILE__), __LINE__, a )//Savy for unicode
#define free( a )      ifree(_T(__FILE__), __LINE__, a ) //Savy for unicode
#endif



#ifndef WIN32
void *icalloc2(int iVal, int iLine, size_t iSize, size_t iLen ) ;
void ifree2(int iVal, int iLine, void* ptr );
void *imalloc2(int iVal, int iLine, size_t iSize );
#else
CLASS_DECL_ZTOOLSO void *icalloc(csprochar* pszFileName, int iLine, size_t iSize, size_t iLen );
CLASS_DECL_ZTOOLSO void ifree(csprochar* pszFileName, int iLine, void* ptr );
CLASS_DECL_ZTOOLSO void *imalloc(csprochar* pszFileName, int iLine, size_t iSize );
#endif
