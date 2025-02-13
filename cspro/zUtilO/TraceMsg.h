#pragma once

// TraceMsg.h
//
// Some definitions useful to generate diagnostics messages when compiling
// rcl, 2005

 #define DebugMsg( a, b )
 #define DebugMessage( b )

#define BOOL2STRING(b) ((b)==true?_T("true"):_T("false"))

#define INDENT _T(" ")
#define INDENT2 INDENT INDENT
#define INDENT3 INDENT2 INDENT

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#ifdef __LOC__
 #undef __LOC__
#endif
#define __LOC__ __FILE__ "(" __STR1__(__LINE__) ") : RCL Warning: "
#define __TODO__ __FILE__ "(" __STR1__(__LINE__) ") : TODO: "

// Use these definitions like this
// #pragma message(__LOC__ "important part to be changed")
