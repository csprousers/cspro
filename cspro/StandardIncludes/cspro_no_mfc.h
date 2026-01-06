#pragma once


// --------------------------------------------------------------------------
// WINDOWS FUNCTIONALITY
// --------------------------------------------------------------------------

#include <Windows.h>


// --------------------------------------------------------------------------
// ASSERTS:
//     - inclusion of assert and definition of ASSERT
// --------------------------------------------------------------------------

#ifndef ASSERT

#include <cassert>

#ifdef _DEBUG
#   define ASSERT(expr) assert(expr)
#else
#   define ASSERT(expr) ((void)0)
#endif

#endif // !ASSERT


// --------------------------------------------------------------------------
// COMMON FUNCTIONALITY
// --------------------------------------------------------------------------

#include <StandardIncludes/cspro_shared.h>
