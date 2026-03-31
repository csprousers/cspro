#pragma once


// --------------------------------------------------------------------------
// STANDARD LIBRARY
// --------------------------------------------------------------------------

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <variant>
#include <vector>


// --------------------------------------------------------------------------
// WARNINGS:
//     - disable ABI-related warnings that are not relevant since the
//       codebase is built with the same compiler / toolset / runtime library
// --------------------------------------------------------------------------

#pragma warning(disable:4251) // 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable:4275) // non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'


// --------------------------------------------------------------------------
// CSPRO-SPECIFIC DEFINITIONS
// --------------------------------------------------------------------------

#define csprochar TCHAR


// --------------------------------------------------------------------------
// COMMON FUNCTIONALITY
// --------------------------------------------------------------------------

#include <StandardIncludes/minimal.h>

#include <zToolsO/StandardTemplatesCpp20.h>

#include <zToolsO/BinaryBlock.h>
#include <zToolsO/CSProException.h>
#include <zToolsO/ErrorMessageDisplayer.h>
#include <zToolsO/InterfaceString.h>
#include <zToolsO/OperatingSystem.h>
#include <zToolsO/PointerClasses.h>
#include <zToolsO/StandardTemplates.h>
#include <zToolsO/StringOperations.h>
#include <zToolsO/SharableString.h>
