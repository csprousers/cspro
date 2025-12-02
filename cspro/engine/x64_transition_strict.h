#pragma once


#ifdef X64_BUILD

// --------------------------------------------------------------------------
// X64_TODO: To support migrating the codebase to x64, some warnings
// are modified to be errors.
// --------------------------------------------------------------------------

#if defined(_DEBUG) && defined(WIN_DESKTOP)

#pragma warning(error:4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(error:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(error:4312) // 'operation' : conversion from 'type1' to 'type2' of greater size
#pragma warning(error:4389) // 'equality-operator' : signed/unsigned mismatch

#endif


#endif // X64_BUILD
