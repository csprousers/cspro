#pragma once

//***************************************************************************
//  File name: ArrUtil.h
//
//  Description:
//       Misc utility functions for working with arrays.
//
//***************************************************************************

#include <zToolsO/VectorHelpers.h>
#include <iterator>


// find intersection of items in 2 arrays, note that this is not
// efficient for large arrays, not even close.
template <class ARRTYPE>
void IntersectArrays(ARRTYPE& dst, const ARRTYPE& src1, const ARRTYPE& src2)
{
    for (int i = 0; i < src1.GetSize(); ++i) {
        if (FindInArray(src2, src1[i]) != -1 && FindInArray(dst, src1[i]) == -1) {
            dst.Add(src1[i]);
        }
    }
    for (int i = 0; i < src2.GetSize(); ++i) {
        if (FindInArray(src1, src2[i]) != -1 && FindInArray(dst, src2[i]) == -1) {
            dst.Add(src2[i]);
        }
    }
}

// return index of an element in an array (CArray or other with GetSize and GetAt defined)
template <class ARRTYPE, class ELTYPE>
inline int FindInArray( const ARRTYPE& arr, const ELTYPE& el)
{
    for (int i = 0; i < arr.GetSize(); ++i) {
        if (arr.GetAt(i) == el) {
            return i;
        }
    }

    return -1;
}

// append items from src to dest ignoring items already in dst.

template <class ARRTYPE>
void AppendUnique(ARRTYPE& dst, const ARRTYPE& src)
{
    for (int i = 0; i < src.GetSize(); ++i) {
        if (FindInArray(dst, src[i]) == -1) {
            dst.Add(src[i]);
        }
    }
}

#ifdef WIN_DESKTOP
inline void AppendUnique(CStringArray& dst, const std::vector<CString>& src)
{
    for (const CString& val : src) {
        if (FindInArray(dst, val) == -1) {
            dst.Add(val);
        }
    }
}
#endif


template<typename T>
typename std::vector<T>::const_iterator FindStringInVectorNoCase(const std::vector<T>& strings, wstring_view search_string)
{
    return std::find_if(strings.cbegin(), strings.cend(),
                        [&search_string](const T& string) { return SO::EqualsNoCase(string, search_string); });
}


template<typename T>
bool ContainsStringInVectorNoCase(const std::vector<T>& strings, wstring_view search_string)
{
    return ( FindStringInVectorNoCase(strings, search_string) != strings.cend() );
}


template<typename T>
void RemoveDuplicateStringsInVectorNoCase(std::vector<T>& strings)
{
    if( strings.size() < 2 )
        return;

    for( size_t i = strings.size() - 1; i > 0; --i )
    {
        const auto& this_string = strings.begin() + i;
        const auto& string_search = FindStringInVectorNoCase(strings, *this_string);

        if( this_string != string_search )
            strings.erase(this_string);
    }
}


template<typename T>
bool IsFilenameInUse(const std::vector<T>& objects_with_filenames, wstring_view filename)
{
    const auto& lookup = std::find_if(objects_with_filenames.cbegin(), objects_with_filenames.cend(),
                                     [&](const auto& object) { return SO::EqualsNoCase(GetUnderlyingValue(object).GetFilename(), filename); });
    return ( lookup != objects_with_filenames.cend() );
}


inline std::vector<std::wstring> CS2WS_Vector(const std::vector<CString>& cstring_values)
{
    std::vector<std::wstring> wstring_values;

    std::transform(cstring_values.cbegin(), cstring_values.cend(),
                   std::back_inserter(wstring_values), [](const CString& value) { return CS2WS(value); });

    return wstring_values;
}


inline std::vector<CString> WS2CS_Vector(const std::vector<std::wstring>& wstring_values)
{
    std::vector<CString> cstring_values;

    std::transform(wstring_values.cbegin(), wstring_values.cend(),
                   std::back_inserter(cstring_values), [](const std::wstring& value) { return WS2CS(value); });

    return cstring_values;
}


#ifdef WIN_DESKTOP
template<typename T, typename ARG_TYPE>
void VectorToCArray(const std::vector<T>& v, CArray<T, ARG_TYPE>& ca)
{
    for( const T& v_val : v )
        ca.Add(v_val);
}
#endif
