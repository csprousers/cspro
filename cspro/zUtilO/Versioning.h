#pragma once

#include <zUtilO/zUtilO.h>


enum class ReleaseType { Alpha, Beta, ReleaseCandidate, Release };


class Versioning
{
public:
    static constexpr double      Number             =  8.1;
    static constexpr const char* NumberText         = "8.1";
    static constexpr const char* NumberDetailedText = "8.1.1";
    static constexpr const char* CSProVersionText   = "CSPro 8.1";

    static constexpr ReleaseType ReleaseType        = ReleaseType::Release;
    static constexpr bool        IsPrerelease       = ( ReleaseType != ::ReleaseType::Release );

    CLASS_DECL_ZUTILO static int GetReleaseDate();
    CLASS_DECL_ZUTILO static std::string GetReleaseDateString();

    CLASS_DECL_ZUTILO static std::string GetVersionString(bool include_cspro = false);
    CLASS_DECL_ZUTILO static std::string GetVersionDetailedString(bool include_cspro = false);

    // Returns one of: "-alpha", "-beta", "-rc", or "", with or without the hyphen.
    CLASS_DECL_ZUTILO static const char* GetReleaseIdentifier(::ReleaseType release_type, bool with_hyphen_prefix);
    CLASS_DECL_ZUTILO static const char* GetReleaseIdentifier(bool with_hyphen_prefix);

private:
    CLASS_DECL_ZUTILO static std::string GetVersionString(std::string version, bool include_cspro);
};
