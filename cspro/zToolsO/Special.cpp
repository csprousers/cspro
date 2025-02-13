#include "StdAfx.h"
#include "Special.h"
#include <array>


double Power10[] =
{
     1E+00, 1E+01, 1E+02, 1E+03, 1E+04, 1E+05, 1E+06, 1E+07, 1E+08,
     1E+09, 1E+10, 1E+11, 1E+12, 1E+13, 1E+14, 1E+15, 1E+16
};

double SpecialValues::m_MISSING;
double SpecialValues::m_NOTAPPL;
double SpecialValues::m_DEFAULT;
double SpecialValues::m_MASKBLK;
double SpecialValues::m_INVALIDINDEX;
double SpecialValues::m_REFUSED;


namespace
{
    static SpecialValues SpecialValuesInstance;

    const TCHAR* const MissingString[] = { _T("MISSING"), _T("Missing") };
    const TCHAR* const DefaultString[] = { _T("DEFAULT"), _T("Default") };
    const TCHAR* const NotApplString[] = { _T("NOTAPPL"), _T("NotAppl") };
    const TCHAR* const RefusedString[] = { _T("REFUSED"), _T("Refused") };

    const std::tuple<wstring_view, double> NameValueMapping[] =
    {
        { MissingString[0], MISSING },
        { DefaultString[0], DEFAULT },
        { NotApplString[0], NOTAPPL },
        { RefusedString[0], REFUSED },
    };
}


SpecialValues::SpecialValues()
{
    std::tuple<double&, std::array<uint8_t, sizeof(double)>> SpecialValuesBinary[] =
    {
        { MISSING,      /* 1.0e50 */ { 0x99, 0x64, 0x7e, 0xc5, 0x0e, 0x1b, 0x51, 0x4a } },
        { NOTAPPL,      /* 1.0e51 */ { 0xbf, 0xfd, 0xdd, 0x76, 0xd2, 0x61, 0x85, 0x4a } },
        { DEFAULT,      /* 1.0e52 */ { 0x2f, 0x7d, 0x95, 0x14, 0x47, 0xba, 0xba, 0x4a } },
        { MASKBLK,      /* 1.0e53 */ { 0x3d, 0x6e, 0xdd, 0x6c, 0x6c, 0xb4, 0xf0, 0x4a } },
        { INVALIDINDEX, /* 1.0e54 */ { 0xce, 0xc9, 0x14, 0x88, 0x87, 0xe1, 0x24, 0x4b } },
        { REFUSED,      /* 1.0e55 */ { 0x41, 0xfc, 0x19, 0x6a, 0xe9, 0x19, 0x5a, 0x4b } },
    };

    for( auto& [special_value, bytes] : SpecialValuesBinary )
    {
        memcpy(reinterpret_cast<uint8_t*>(&special_value), bytes.data(), bytes.size());
        ASSERT(IsSpecial(special_value));
    }
}


template<typename T>
T SpecialValues::StringIsSpecial(const wstring_view text_sv)
{
    if( !text_sv.empty() && std::iswalpha(text_sv.front()) )
    {
        for( size_t i = 0; i < _countof(NameValueMapping); ++i )
        {
            if( SO::EqualsNoCase(text_sv, std::get<0>(NameValueMapping[i])) )
            {
                if constexpr(std::is_same_v<T, bool>)
                {
                    return true;
                }

                else if constexpr(std::is_same_v<T, const double*>)
                {
                    return &std::get<1>(NameValueMapping[i]);
                }

                else
                {
                    return std::get<1>(NameValueMapping[i]);
                }
            }
        }
    }

    if constexpr(std::is_same_v<T, bool>)
    {
        return false;
    }

    else if constexpr(std::is_same_v<T, const double*>)
    {
        return nullptr;
    }

    else
    {
        return std::nullopt;
    }
}

template CLASS_DECL_ZTOOLSO bool SpecialValues::StringIsSpecial(wstring_view text_sv);
template CLASS_DECL_ZTOOLSO const double* SpecialValues::StringIsSpecial(wstring_view text_sv);
template CLASS_DECL_ZTOOLSO std::optional<double> SpecialValues::StringIsSpecial(wstring_view text_sv);


double SpecialValues::StringToValue(const wstring_view text_sv)
{
    for( size_t i = 0; i < _countof(NameValueMapping); ++i )
    {
        if( SO::EqualsNoCase(text_sv, std::get<0>(NameValueMapping[i])) )
            return std::get<1>(NameValueMapping[i]);
    }

    return ReturnProgrammingError(DEFAULT);
}


const TCHAR* const SpecialValues::ValueToString(double value, bool all_caps_version/* = true*/)
{
    size_t index = all_caps_version ? 0 : 1;

    return ( value == MISSING ) ? MissingString[index] :
           ( value == NOTAPPL ) ? NotApplString[index] :
           ( value == DEFAULT ) ? DefaultString[index] :
           ( value == REFUSED ) ? RefusedString[index] :
                                  ReturnProgrammingError(_T("???????"));
}


bool SpecialValues::CompareForDisplayOrder(double value1, double value2)
{
    // sort things as: non-special, refused, missing, default, notappl
    const static double SpecialOrder[] = { REFUSED, MISSING, DEFAULT, NOTAPPL };

    if( IsSpecial(value1) && IsSpecial(value2) )
    {
        auto get_order = [&](double& value)
        {
            for( size_t i = 0; i < _countof(SpecialOrder); ++i )
            {
                if( value == SpecialOrder[i] )
                {
                    value = i;
                    return;
                }
            }
        };

        get_order(value1);
        get_order(value2);
    };

    return ( value1 < value2 );
}
