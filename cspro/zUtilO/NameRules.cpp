#include "StdAfx.h"
#include "NameRules.h"


bool NameRules::IsName(const std::string_view name_sv) noexcept
{
    auto name_sv_itr = name_sv.cbegin();
    const auto& name_sv_end = name_sv.cend();

    // the first character must be alpha
    if( name_sv_itr == name_sv_end || !is_alpha(*name_sv_itr) )
        return false;

    // additional characters must be alpha, numeric, or an underscore
    while( ++name_sv_itr != name_sv_end )
    {
        if( !is_tokch(*name_sv_itr) )
            return false;
    }

    // the last character cannot be an underscore
    if( *(name_sv_itr - 1) == '_' )
        return false;

    return true;
}


bool NameRules::IsUpperCaseName(const std::string_view name_sv) noexcept
{
    return ( SO::IsUpper(name_sv) && IsName(name_sv) );
}
