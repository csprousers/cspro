#pragma once

// for more serious warning checking, make some warnings errors

#ifdef WIN32

#pragma warning(error:4005) // macro redefinition
#pragma warning(error:4150) // deletion of pointer to incomplete type 'type'; no destructor called
#pragma warning(error:4172) // returning address of local variable or temporary
#pragma warning(error:4239) // nonstandard extension used : 'token' : conversion from 'type' to 'type'
#pragma warning(error:4456) // declaration of 'identifier' hides previous local declaration
#pragma warning(error:4702) // unreachable code
#pragma warning(error:4715) // 'function': not all control paths return a value
#pragma warning(error:4717) // 'function' : recursive on all control paths, function will cause runtime stack overflow
#pragma warning(error:4834) // discarding return value of function with 'nodiscard' attribute
#pragma warning(error:5205) // delete of an abstract class 'type-name' that has a non-virtual destructor results in undefined behavior

// issue some warnings as errors only in debug mode (on the desktop)
#if defined(_DEBUG) && defined(WIN_DESKTOP)
#pragma warning(error:4100) // 'identifier': unreferenced formal parameter
#pragma warning(error:4189) // local variable is initialized but not referenced
#endif

#endif
