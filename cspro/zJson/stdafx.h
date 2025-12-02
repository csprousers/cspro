#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/StrictCompilerErrors.h>
// X64_TODO #include <engine/x64_transition_strict.h>

#include <zToolsO/Tools.h>
#include <external/jsoncons/json.hpp>

#ifndef JSONCONS_NO_DEPRECATED
static_assert(false, "uncomment line 21 of external/jsoncons/config/compiler_support.hpp");
#endif
