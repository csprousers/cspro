#pragma once

#include <zUtilO/zUtilO.h>


class Versioning
{
public:
    static constexpr double Number =                       8.0;
    static constexpr const char* NumberText =             "8.0";
    static constexpr const char* NumberDetailedText =     "8.0.1";
    static constexpr const char* CSProVersionText = "CSPro 8.0";

    static constexpr bool IsBeta = false;


    CLASS_DECL_ZUTILO static int GetReleaseDate();
    CLASS_DECL_ZUTILO static std::string GetReleaseDateString();

    CLASS_DECL_ZUTILO static std::string GetVersionString(bool include_cspro = false);
    CLASS_DECL_ZUTILO static std::string GetVersionDetailedString(bool include_cspro = false);

private:
    CLASS_DECL_ZUTILO static std::string GetVersionString(std::string version, bool include_cspro);
};
