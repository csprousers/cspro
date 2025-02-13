#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/StrictCompilerErrors.h>

#include <zToolsO/Utf8Convert.h>
#include <zUtilO/imsaStr.h>

#include <external/jsoncons/json.hpp>

#ifndef JSONCONS_NO_DEPRECATED
static_assert(false, "uncomment line 21 of external/jsoncons/config/compiler_support.hpp");
#endif
