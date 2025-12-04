#pragma once


// in debug: performs a dynamic_cast and asserts that the cast succeeded
// in release: performs a static_cast

// assert_cast does not allow null inputs
// assert_nullable_cast does


template<typename To, typename From, bool AllowNull = false>
To assert_cast(From&& from)
{
#ifdef _DEBUG
    if constexpr(std::is_pointer_v<To>)
    {
        To to = dynamic_cast<To>(from);

        ASSERT(to == static_cast<To>(from));

        if constexpr(AllowNull)
        {
            ASSERT(to != nullptr || from == nullptr);
        }

        else
        {
            ASSERT(to != nullptr);
        }

        return to;
    }

    else
    {
        static_assert(std::is_reference_v<To>);
        typedef typename std::remove_reference<To>::type To_Type;
        To_Type* to = dynamic_cast<To_Type*>(&from);
        ASSERT(to != nullptr && to == static_cast<To_Type*>(&from));
        return dynamic_cast<To>(from);
    }

#else
    return static_cast<To>(from);

#endif
}


template<typename To, typename From>
To assert_nullable_cast(From from)
{
    static_assert(std::is_pointer_v<To> && std::is_pointer_v<From>);
    return assert_cast<To, From, true>(std::forward<From>(from));
}
