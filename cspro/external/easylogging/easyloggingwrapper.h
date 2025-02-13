#pragma once
// Include this file instead of easylogging++.h to avoid macro pollution
#pragma push_macro("DEBUG")
#pragma push_macro("INFO")
#pragma push_macro("WARNING")
#pragma push_macro("ERROR")
#pragma push_macro("FATAL")
#pragma push_macro("TRACE")
#pragma push_macro("VERBOSE")

#include <easylogging++.h>

#pragma pop_macro("DEBUG")
#pragma pop_macro("INFO")
#pragma pop_macro("WARNING")
#pragma pop_macro("ERROR")
#pragma pop_macro("FATAL")
#pragma pop_macro("TRACE")
#pragma pop_macro("VERBOSE")
