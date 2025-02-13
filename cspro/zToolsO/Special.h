#pragma once

#include <zToolsO/zToolsO.h>


#define MAGICROUND      0.5000000001   // Magic number for perfect rounding
#define MAX_NUMLEN      15             // Max for numeric fields

CLASS_DECL_ZTOOLSO extern double Power10[MAX_NUMLEN + 2];


// Special values (Missing, NotAppl, Default, Refused)
class CLASS_DECL_ZTOOLSO SpecialValues
{
public:
     SpecialValues();

     static double m_MISSING;
     static double m_NOTAPPL;
     static double m_DEFAULT;
     static double m_MASKBLK;
     static double m_INVALIDINDEX;
     static double m_REFUSED;

     // can return: bool, const double*, std::optional<double>
     template<typename T = bool>
     static T StringIsSpecial(wstring_view text_sv);

     static double StringToValue(wstring_view text_sv);
     static const TCHAR* const ValueToString(double value, bool all_caps_version = true);

     static double SmallestSpecialValue() { return m_MISSING; }
     static double LargestSpecialValue()  { return m_REFUSED; }

     static bool CompareForDisplayOrder(double value1, double value2);
};


#define MAXVALUE     SpecialValues::m_MISSING
#define MISSING      SpecialValues::m_MISSING
#define NOTAPPL      SpecialValues::m_NOTAPPL
#define DEFAULT      SpecialValues::m_DEFAULT
#define MASKBLK      SpecialValues::m_MASKBLK
#define INVALIDINDEX SpecialValues::m_INVALIDINDEX // RHF Sep 21, 2001 Relation
#define REFUSED      SpecialValues::m_REFUSED


inline bool IsSpecial(double value)
{
    return ( value >= MAXVALUE );
}
