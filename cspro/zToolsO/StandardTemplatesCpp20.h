#pragma once


// --------------------------------------------------------------------------
// C++20 functionality to be removed once we move on from C++17
// --------------------------------------------------------------------------

#if ( !defined(_MSVC_LANG) && __cplusplus < 202002L ) || ( _MSVC_LANG < 202002L )

namespace std
{
    // std::make_unique_for_overwrite
    template<typename T>
    std::unique_ptr<T> make_unique_for_overwrite(const size_t size)
    {
        // from https://stackoverflow.com/questions/45703152/recommended-way-to-make-stdunique-ptr-of-array-type-without-value-initializati
        return unique_ptr<T>(new typename std::remove_extent<T>::type[size]);
    }


    // std::remove_cvref + std::remove_cvref_t
    template<typename T> struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};
    template<typename T> using remove_cvref_t = typename std::remove_cvref<T>::type;
}


template<bool flag = false>
void static_assert_false()
{
#ifndef WASM
    static_assert(flag);
#endif
}

#endif
