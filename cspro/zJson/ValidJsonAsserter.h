#pragma once

#include <zJson/Json.h>


#ifdef _DEBUG

inline void AssertValidJson(const wstring_view json_text_sv)
{
    try
    {
        Json::Parse(json_text_sv);
    }

    catch( const JsonParseException& exception )
    {
        ASSERT(false);
        ErrorMessage::Display(exception);
    }
}

#else

#define AssertValidJson(json_text_sv) ((void)0)

#endif


template<typename T>
T AssertAndReturnValidJson(T&& json_text)
{
    AssertValidJson(json_text);
    return std::forward<T>(json_text);
}
