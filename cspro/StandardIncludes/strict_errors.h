#pragma once


#ifdef WIN32

// for more serious warning checking, make some warnings errors:

#pragma warning(error:4005) // macro redefinition
#pragma warning(error:4114) // same type qualifier used more than once
#pragma warning(error:4150) // deletion of pointer to incomplete type 'type'; no destructor called
#pragma warning(error:4172) // returning address of local variable or temporary
#pragma warning(error:4238) // nonstandard extension used: class rvalue used as lvalue
#pragma warning(error:4239) // nonstandard extension used : 'token' : conversion from 'type' to 'type'
#pragma warning(error:4297) // 'function' : function assumed not to throw an exception but does
#pragma warning(error:4456) // declaration of 'identifier' hides previous local declaration
#pragma warning(error:4457) // declaration of 'identifier' hides function parameter
#pragma warning(error:4552) // 'operator': result of expression not used
#pragma warning(error:4715) // 'function': not all control paths return a value
#pragma warning(error:4717) // 'function' : recursive on all control paths, function will cause runtime stack overflow
#pragma warning(error:4834) // discarding return value of function with 'nodiscard' attribute
#pragma warning(error:5205) // delete of an abstract class 'type-name' that has a non-virtual destructor results in undefined behavior


// issue some warnings as errors only in debug mode (on the desktop):

#if defined(_DEBUG) && defined(WIN_DESKTOP)
#pragma warning(error:4100) // 'identifier': unreferenced formal parameter
#pragma warning(error:4101) // 'identifier': unreferenced local variable
#pragma warning(error:4189) // local variable is initialized but not referenced
#pragma warning(error:4702) // unreachable code
#endif


// some warnings will only be issued when building for x64:

#if defined(X64_BUILD) && !defined(DISABLE_64_BIT_ERRORS)
#pragma warning(error:4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(error:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(error:4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
#pragma warning(error:4389) // 'equality-operator' : signed/unsigned mismatch
#endif


#endif // WIN32
