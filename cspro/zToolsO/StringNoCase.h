#pragma once

#include <zToolsO/StringOperations.h>


// --------------------------------------------------------------------------
// StringNoCase
//
// a class that maintains the case of a string, but when used in
// maps or sets, returns a case insensitive comparison
// --------------------------------------------------------------------------

class StringNoCase : public std::wstring
{
public:
    template<typename... Args>
    StringNoCase(Args&&... args) 
        :   std::wstring(std::forward<Args>(args)...)
    {
    }

    bool operator==(const StringNoCase& rhs) const
    {
        return SO::EqualsNoCase(*this, rhs);
    }

    bool operator<(const StringNoCase& rhs) const noexcept
    {
        return ( SO::CompareNoCase(*this, rhs) < 0 );
    }
};
