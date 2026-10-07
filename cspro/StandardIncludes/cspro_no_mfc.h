#pragma once


// --------------------------------------------------------------------------
// WINDOWS FUNCTIONALITY
// --------------------------------------------------------------------------

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#else

#define MINIMAL_WINDOWS_DEFINES
#include <zPlatformO/PortableWindowsDefines.h>

#endif


// --------------------------------------------------------------------------
// ASSERT + TRACE
//
// CSLOG, in zToolsO, will generally handle ASSERT and TRACE statements.
//
// If NO_CSLOG_USING_DLLS is set in a DLL-based compilation:
//   - ASSERT will use assert.
//   - TRACE will not be defined.
// --------------------------------------------------------------------------

#ifndef ASSERT
#  ifdef _DEBUG
#    if defined(NO_CSLOG_USING_DLLS) && !defined(CSPRO_NO_DLLS)
#      include <cassert>
#      define ASSERT(f) assert((f))
#    else
#      include <zToolsO/DebugLogging.h>
#      define ASSERT(f) CSLOG_ASSERT(f)
#    endif
#  else // RELEASE
#    define ASSERT(f) ((void)0)
#  endif
#endif

#if !defined(TRACE) && !defined(NO_CSLOG)
#  ifdef _DEBUG
#    include <zToolsO/DebugLogging.h>
#    define TRACE CSLOG_TRACE
#  else // RELEASE
#    define TRACE(...) ((void)0)
#  endif
#endif


// --------------------------------------------------------------------------
// CString
//
// Until CString is removed from shared code, we will forward declare it to
// allow the compilation of header files in MFC-based projects.
// --------------------------------------------------------------------------

#ifdef USING_CSTRING
static_assert(false);
#else
class CString;
#endif


// --------------------------------------------------------------------------
// COMMON FUNCTIONALITY
// --------------------------------------------------------------------------

#include <StandardIncludes/cspro_shared.h>
