#pragma once

#include <optional>

namespace cs { template<typename T> class cref_optional; }


// --------------------------------------------------------------------------
// cs::cref_optional
//
// This class if similar to std::optional except that it stores a pointer
// to an object whose lifetime must outlive the lifetime of this object.
//
// The object can also point to nothing, and an object will be created when
// trying to emplace or assign.
//
// The goal of this class is wrap objects, typically function parameters,
// that are unlikely to be modified.
//
// An object can be created from a pointer by using the static FromPointer
// method.
// --------------------------------------------------------------------------

template<typename T>
class cs::cref_optional
{
public:
    cref_optional(const T& value)
        :   m_value(&value)
    {
    }

    cref_optional()
        :   m_value(nullptr)
    {
    }

    cref_optional(std::nullopt_t)
        :   m_value(nullptr)
    {
    }

    cref_optional(const cref_optional& rhs)
        :   m_value(rhs.m_value)
    {
    }

    cref_optional(cref_optional&& rhs)
        :   m_value(rhs.m_value),
            m_modifiedValue(std::move(rhs.m_modifiedValue))
    {
    }

    static cref_optional FromPointer(const T* value)
    {
        return ( value != nullptr ) ? cref_optional<T>(*value) :
                                        cref_optional<T>();
    }

    template<typename RT>
    cref_optional<T>& operator=(RT&& value)
    {
        if( m_modifiedValue == nullptr )
        {
            m_modifiedValue = std::make_unique<T>(std::forward<RT>(value));
            m_value = m_modifiedValue.get();
        }

        else
        {
            *m_modifiedValue = std::forward<RT>(value);
        }

        return *this;
    }

    cref_optional<T>& operator=(std::optional<T>&& value)
    {
        if( value.has_value() )
        {
            return operator=(std::move(*value));
        }

        else
        {
            m_value = nullptr;
            m_modifiedValue.reset();
            return *this;
        }
    }

    template<typename... Args>
    cref_optional<T>& emplace(Args&&... args)
    {
        m_modifiedValue = std::make_unique<T>(std::forward<Args>(args)...);
        m_value = m_modifiedValue.get();
        return *this;
    }

    [[nodiscard]] bool has_value() const
    {
        return ( m_value != nullptr );
    }

    [[nodiscard]] const T* get() const
    {
        return m_value;
    }

    [[nodiscard]] const T* operator->() const
    {
        ASSERT(m_value != nullptr);
        return m_value;
    }

    [[nodiscard]] const T& operator*() const
    {
        return *operator->();
    }

private:
    const T* m_value;
    std::unique_ptr<T> m_modifiedValue;
};
