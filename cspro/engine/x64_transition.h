#pragma once


#if INTPTR_MAX == INT64_MAX
#define X64_BUILD
constexpr bool IsX64() { return true; }
#else
constexpr bool IsX64() { return false; }
#endif
