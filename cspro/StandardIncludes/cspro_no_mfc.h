#pragma once


// --------------------------------------------------------------------------
// WINDOWS FUNCTIONALITY
// --------------------------------------------------------------------------

#define NOMINMAX
#include <Windows.h>


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
// COMMON FUNCTIONALITY
// --------------------------------------------------------------------------

#include <StandardIncludes/cspro_shared.h>
