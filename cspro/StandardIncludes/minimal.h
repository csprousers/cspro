#pragma once


// --------------------------------------------------------------------------
// ASSERTS:
//     - assert definitions that can be disabled in future versions
// --------------------------------------------------------------------------

#define ASSERT80(f) ASSERT(f)
#define ASSERT81(f) ASSERT(f)



// --------------------------------------------------------------------------
// WARNINGS -> ERRORS:
//     - for more serious warning checking, make some warnings errors
// --------------------------------------------------------------------------

#ifdef WIN32
#pragma warning(error:4005) // macro redefinition
#pragma warning(error:4150) // deletion of pointer to incomplete type 'type'; no destructor called
#pragma warning(error:4311) // 'variable' : pointer truncation from 'type' to 'type'
#pragma warning(error:4840) // non-portable use of class 'type' as an argument to a variadic function
#endif



// --------------------------------------------------------------------------
// DEBUGGING:
//     - harmonize the DEBUG and _DEBUG preprocessor definitions
//     - add DebugMode for constexpr evaluation
// --------------------------------------------------------------------------

#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG
#elif defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif


constexpr bool DebugMode()
{
#ifdef _DEBUG
    return true;
#else
    return false;
#endif
}


// --------------------------------------------------------------------------
// COMMON FUNCTIONALITY:
//     - forward declarations of commonly used classes
//     - headers used throughout the projects
// --------------------------------------------------------------------------

class JsonNode;
class JsonWriter;
class Serializer;


#include <StandardIncludes/x64_transition.h>
#include <StandardIncludes/assert_cast.h>
