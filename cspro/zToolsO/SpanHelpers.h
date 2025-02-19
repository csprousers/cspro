#pragma once

#include <zToolsO/PointerType.h>
#include <zToolsO/span.h>


class SpanHelpers
{
public:
    // returns an object, either a cs::span or a std::vector, that points to the objects in the vector, calculated using the GetPointer function;
    // because the returned value may be a std::vector, the input values vector must outlive the returned object,
    // and the value should not be directly assigned to a cs::span object (unless as a function argument)
    template<typename T> static auto CreatePointersSpan(std::vector<T>& values)       { return CreatePointersSpanWorker<T>(values, values.size()); }
    template<typename T> static auto CreatePointersSpan(const std::vector<T>& values) { return CreatePointersSpanWorker<T>(values, values.size()); }

    // same as the above, but with the ability to only use some number of elements
    template<typename T> static auto CreatePointersSpan(std::vector<T>& values, size_t count)       { return CreatePointersSpanWorker<T>(values, count); }
    template<typename T> static auto CreatePointersSpan(const std::vector<T>& values, size_t count) { return CreatePointersSpanWorker<T>(values, count); }

private:
    template<typename PT, typename VT>
    static std::vector<PT> CreateVectorOfPointers(VT& values, size_t count);

    template<typename T, typename VT>
    static auto CreatePointersSpanWorker(VT& values, size_t count);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename PT, typename VT>
std::vector<PT> SpanHelpers::CreateVectorOfPointers(VT& values, const size_t count)
{
    ASSERT(count <= values.size());

    std::vector<PT> pointers;
    pointers.reserve(count);

    auto values_itr = values.begin();
    const auto& values_end = values_itr + count;

    for( ; values_itr != values_end; ++values_itr )
        pointers.emplace_back(GetPointer(*values_itr));

    return pointers;
}


template<typename T, typename VT>
auto SpanHelpers::CreatePointersSpanWorker(VT& values, const size_t count)
{
    ASSERT(count <= values.size());

    // raw pointers
    if constexpr(std::is_pointer_v<T>)
    {
        return cs::span<T>(values.data(), count);
    }

    // smart pointers
    else if constexpr(IsPointer<T>())
    {
        using PT = typename std::conditional<std::is_const_v<VT>, const typename T::element_type*, typename T::element_type*>::type;
        return CreateVectorOfPointers<PT>(values, count);
    }

    // values
    else
    {
        using PT = typename std::conditional<std::is_const_v<VT>, const T*, T*>::type;
        return CreateVectorOfPointers<PT>(values, count);
    }
}
