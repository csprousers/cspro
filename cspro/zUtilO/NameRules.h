#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// NameRules
//
// This class provides static methods for processing CSPro names.
//
// CSPro names inherit the rules from IMSA and can contain:
//
// - Letters (A-Z)
// - Digits (0-9)
// - Underscores
//
// The name must start with a letter and cannot end in an underscore.
//
// IMSA only allowed names up to 16 characters, but CSPro names can be of
// any length.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO NameRules
{
public:
    // Returns true if the name is valid.
    static bool IsName(std::string_view name_sv) noexcept;

    // Returns true if the name is both valid and defined in upper case.
    static bool IsUpperCaseName(std::string_view name_sv) noexcept;
};
