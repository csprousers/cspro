#pragma once

// --------------------------------------------------------------------------
//
// Several templated classes are defined to faciliate working with vectors
// of shared pointers as if they were values.
//
// SharedPointerAsValueVector
//     A subclass of a vector of shared pointers whose copy constructor does
//     not copy the shared pointers but creates new shared pointers of values
//     from the values.
//
// SharedPointerVectorWrapper
//     A wrapper around a vector of shared pointers that provides an iterator
//     that allows access to the value, not the shared pointer.
//
// --------------------------------------------------------------------------



// --------------------------------------------------------------------------
// SharedPointerAsValueVector
// --------------------------------------------------------------------------

template<typename T>
class SharedPointerAsValueVector : public std::vector<std::shared_ptr<T>>
{
    using BaseVectorT = std::vector<std::shared_ptr<T>>;
    using NonConstT = std::remove_const_t<T>;

public:
    SharedPointerAsValueVector() noexcept
    {
    }

    SharedPointerAsValueVector(const SharedPointerAsValueVector<T>& rhs)
    {
        for( const auto& value_pointer : static_cast<const BaseVectorT&>(rhs) )
            BaseVectorT::emplace_back(std::make_shared<T>(static_cast<const T&>(*value_pointer)));
    }

    SharedPointerAsValueVector(SharedPointerAsValueVector<T>&& rhs) noexcept
        :   BaseVectorT(std::move(rhs))
    {
    }

    SharedPointerAsValueVector(BaseVectorT&& rhs) noexcept
        :   BaseVectorT(std::move(rhs))
    {
    }

    SharedPointerAsValueVector<T>& operator=(const SharedPointerAsValueVector<T>& rhs)
    {
        BaseVectorT::clear();

        for( const auto& value_pointer : static_cast<const BaseVectorT&>(rhs) )
            BaseVectorT::emplace_back(std::make_shared<T>(static_cast<const T&>(*value_pointer)));

        return *this;
    };

    SharedPointerAsValueVector<T>& operator=(SharedPointerAsValueVector<T>&& rhs) noexcept
    {
        BaseVectorT::operator=(static_cast<BaseVectorT&&>(std::move(rhs)));
        return *this;
    }

    SharedPointerAsValueVector<T>& operator=(BaseVectorT&& rhs) noexcept
    {
        BaseVectorT::operator=(std::move(rhs));
        return *this;
    }

    operator std::vector<NonConstT>() const
    {
        std::vector<NonConstT> values;

        for( const auto& value_pointer : *this )
            values.emplace_back(static_cast<const T&>(*value_pointer));

        return values;
    }

    template<typename ST>
    void serialize(ST& ar)
    {
        if( ar.IsSaving() )
        {
            ar.template Write<size_t>(BaseVectorT::size());
        }

        else
        {
            BaseVectorT::resize(ar.template Read<size_t>());
        }

        for( auto& value_pointer : *this )
        {
            if( ar.IsLoading() )
                value_pointer = std::make_shared<T>();

            ar & *value_pointer;
        }
    }
};



// --------------------------------------------------------------------------
// SharedPointerVectorWrapper
// --------------------------------------------------------------------------

template<typename, bool, bool, bool> class SharedPointerVectorWrapper;

template<typename AccessType, typename VectorType>
class SharedPointerValueIterator
{
    template<typename, bool, bool, bool> friend class SharedPointerVectorWrapper;

private:
    SharedPointerValueIterator(const std::shared_ptr<VectorType>* iterator)
        :   m_iterator(iterator)
    {
    }

public:
    [[nodiscard]] bool operator==(const SharedPointerValueIterator<AccessType, VectorType>& rhs) const
    {
        return ( m_iterator == rhs.m_iterator );
    }

    [[nodiscard]] bool operator!=(const SharedPointerValueIterator<AccessType, VectorType>& rhs) const
    {
        return ( m_iterator != rhs.m_iterator );
    }

    SharedPointerValueIterator<AccessType, VectorType>& operator++()
    {
        ++m_iterator;
        return *this;
    }

    [[nodiscard]] const AccessType* operator->() const
    {
        return m_iterator->get();
    }

    [[nodiscard]] AccessType* operator->()
    {
        return m_iterator->get();
    }

    [[nodiscard]] const AccessType& operator*() const
    {
        return *operator->();
    }

    [[nodiscard]] AccessType& operator*()
    {
        return *operator->();
    }

private:
    const std::shared_ptr<VectorType>* m_iterator;
};


template<typename T,
         bool AllowNonConstAccessToSharedPointers = false,
         bool AllowVectorModification = false,
         bool AllowCloningValues = true>
class SharedPointerVectorWrapper
{
    using NonConstT = std::remove_const_t<T>;
    using ConstT = const T;

    using VectorT = typename std::conditional_t<AllowVectorModification, std::vector<std::shared_ptr<T>>, const std::vector<std::shared_ptr<T>>>;

    using CloneSharedPointersT = typename std::conditional_t<AllowNonConstAccessToSharedPointers, T, ConstT>;

public:
    SharedPointerVectorWrapper(VectorT& vector)
        :   m_vector(vector)
    {
    }


    [[nodiscard]] SharedPointerValueIterator<ConstT, T> begin() const
    {
        return SharedPointerValueIterator<ConstT, T>(m_vector.data());
    }

    [[nodiscard]] SharedPointerValueIterator<ConstT, T> cbegin() const
    {
        return begin();
    }

    [[nodiscard]] SharedPointerValueIterator<T, T> begin()
    {
        return SharedPointerValueIterator<T, T>(m_vector.data());
    }

    [[nodiscard]] SharedPointerValueIterator<ConstT, T> end() const
    {
        return SharedPointerValueIterator<ConstT, T>(m_vector.data() + m_vector.size());
    }

    [[nodiscard]] SharedPointerValueIterator<ConstT, T> cend() const
    {
        return end();
    }

    [[nodiscard]] SharedPointerValueIterator<T, T> end()
    {
        return SharedPointerValueIterator<T, T>(m_vector.data() + m_vector.size());
    }


    // clone the shared pointers (returning const shared pointers by default)
    std::vector<std::shared_ptr<CloneSharedPointersT>> CloneSharedPointers() const
    {
        if constexpr(std::is_same_v<T, CloneSharedPointersT>)
        {
            return m_vector;
        }

        else
        {
            std::vector<std::shared_ptr<CloneSharedPointersT>> value_pointers;

            for( const auto& value_pointer : m_vector )
                value_pointers.emplace_back(value_pointer);

            return value_pointers;
        }
    }


    // access the shared pointer vector (disabled by default)
    const VectorT& GetSharedPointerVector() const
    {
        static_assert(AllowNonConstAccessToSharedPointers || std::is_same_v<T, ConstT>, "access to the shared pointer vector with non-const shared pointers has been disallowed");
        return m_vector;
    }

    VectorT& GetSharedPointerVector()
    {
        static_assert(AllowNonConstAccessToSharedPointers || std::is_same_v<T, ConstT>, "access to the shared pointer vector with non-const shared pointers has been disallowed");
        return m_vector;
    }


    // clone the vector as values
    std::vector<NonConstT> CloneValues() const
    {
        static_assert(AllowCloningValues, "the ability to clone values has been disallowed");

        std::vector<NonConstT> values;

        for( const auto& value_pointer : m_vector )
            values.emplace_back(static_cast<const T&>(*value_pointer));

        return values;
    }

    // clone the vector as shared pointers
    template<typename SPT = NonConstT>
    std::vector<std::shared_ptr<SPT>> CloneValuesAsSharedPointers() const
    {
        static_assert(AllowCloningValues, "the ability to clone values has been disallowed");

        std::vector<std::shared_ptr<SPT>> value_pointers;

        for( const auto& value : m_vector )
            value_pointers.emplace_back(std::make_shared<NonConstT>(static_cast<const T&>(*value)));

        return value_pointers;
    }

private:
    VectorT& m_vector;
};
