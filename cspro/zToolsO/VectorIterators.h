#pragma once

#include <zToolsO/PointerType.h>


// --------------------------------------------------------------------------
//
// VI_P
//     A range-based for loop wrapper around a vector of values, raw
//     pointers, or smart pointers, that provides an iterator that allows
//     access to the (potentially null) value using its pointer.
//
// VI_V
//     A range-based for loop wrapper around a vector of values, raw
//     pointers, or smart pointers, that provides an iterator that allows
//     access to the non-null value.
//
// --------------------------------------------------------------------------

template<typename, typename> class VI_Wrapper;

template<typename ValueType, typename AccessType>
class VI_Iterator
{
    template<typename, typename> friend class VI_Wrapper;

private:
    VI_Iterator(ValueType* iterator)
        :   m_iterator(iterator)
    {
    }

public:
    [[nodiscard]] bool operator==(const VI_Iterator<ValueType, AccessType>& rhs) const
    {
        return ( m_iterator == rhs.m_iterator );
    }

    [[nodiscard]] bool operator!=(const VI_Iterator<ValueType, AccessType>& rhs) const
    {
        return ( m_iterator != rhs.m_iterator );
    }

    VI_Iterator<ValueType, AccessType>& operator++()
    {
        ++m_iterator;
        return *this;
    }

    [[nodiscard]] AccessType operator*()
    {
        if constexpr(std::is_pointer_v<AccessType>)
        {
            return GetPointer(*m_iterator);
        }

        else
        {
            ASSERT(GetPointer(*m_iterator) != nullptr);
            return *GetPointer(*m_iterator);
        }
    }

private:
    ValueType* m_iterator;
};



template<typename VectorType, typename BaseValueType>
struct VI_ApplyVectorConstness
{
    using ValueType = typename std::conditional_t<std::is_const_v<VectorType>, typename std::add_const_t<BaseValueType>,
                                                                               BaseValueType>;
};


template<typename VectorType, typename AccessType>
class VI_Wrapper
{
public:
    VI_Wrapper(VectorType& vector)
        :   m_vector(vector)
    {
    }

    using ValueType = typename VI_ApplyVectorConstness<VectorType, typename VectorType::value_type>::ValueType;

    [[nodiscard]] VI_Iterator<ValueType, AccessType> begin() const { return VI_Iterator<ValueType, AccessType>(m_vector.data()); }
    [[nodiscard]] VI_Iterator<ValueType, AccessType> end() const   { return VI_Iterator<ValueType, AccessType>(m_vector.data() + m_vector.size()); }

private:
    VectorType& m_vector;
};


template<typename VectorType>
[[nodiscard]] auto VI_P(VectorType& vector)
{
    if constexpr(IsPointer<typename VectorType::value_type>())
    {
        return VI_Wrapper<VectorType, typename VI_ApplyVectorConstness<VectorType, typename std::pointer_traits<typename VectorType::value_type>::element_type>::ValueType*>(vector);
    }

    else
    {
        return VI_Wrapper<VectorType, typename VI_ApplyVectorConstness<VectorType, typename VectorType::value_type>::ValueType*>(vector);
    }
}


template<typename VectorType>
[[nodiscard]] auto VI_V(VectorType& vector)
{
    if constexpr(IsPointer<typename VectorType::value_type>())
    {
        return VI_Wrapper<VectorType, typename VI_ApplyVectorConstness<VectorType, typename std::pointer_traits<typename VectorType::value_type>::element_type>::ValueType&>(vector);
    }

    else
    {
        return VI_Wrapper<VectorType, typename VI_ApplyVectorConstness<VectorType, typename VectorType::value_type>::ValueType&>(vector);
    }
}
