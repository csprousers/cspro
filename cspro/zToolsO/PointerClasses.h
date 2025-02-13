#pragma once


namespace cs
{
    // --------------------------------------------------------------------------
    // shared_or_raw_ptr
    // 
    //  - holds a shared or raw pointer
    //
    //  - the implementation is limited, as it is intended to be used by
    //    objects that simply want to hold either type of pointer
    // --------------------------------------------------------------------------

    template<typename T>
    class shared_or_raw_ptr
    {
    public:
        shared_or_raw_ptr() noexcept
            :   m_ptr(nullptr)
        {
        }

        shared_or_raw_ptr(const shared_or_raw_ptr<T>& rhs) noexcept
            :   m_sharedPtr(rhs.m_sharedPtr),
                m_ptr(rhs.m_ptr)
        {
        }

        shared_or_raw_ptr(shared_or_raw_ptr<T>&& rhs) noexcept
            :   m_sharedPtr(std::move(rhs.m_sharedPtr)),
                m_ptr(rhs.m_ptr)
        {
            rhs.m_ptr = nullptr;
        }

        template<typename RT>
        shared_or_raw_ptr(std::shared_ptr<RT> shared_ptr) noexcept
            :   m_sharedPtr(std::move(shared_ptr)),
                m_ptr(m_sharedPtr.get())
        {
        }

        template<typename RT>
        shared_or_raw_ptr(std::unique_ptr<RT> unique_ptr) noexcept
            :   m_sharedPtr(std::move(unique_ptr)),
                m_ptr(m_sharedPtr.get())
        {
        }

        shared_or_raw_ptr(T* raw_ptr) noexcept
            :   m_ptr(raw_ptr)
        {
        }

        shared_or_raw_ptr<T>& operator=(const shared_or_raw_ptr<T>& rhs) noexcept
        {
            m_sharedPtr = rhs.m_sharedPtr;
            m_ptr = rhs.m_ptr;
            return *this;
        }

        shared_or_raw_ptr<T>& operator=(shared_or_raw_ptr<T>&& rhs) noexcept
        {
            m_sharedPtr = std::move(rhs.m_sharedPtr);
            m_ptr = rhs.m_ptr;
            rhs.m_ptr = nullptr;
            return *this;
        }

        template<typename RT>
        shared_or_raw_ptr<T>& operator=(std::shared_ptr<RT> shared_ptr) noexcept
        {
            m_sharedPtr = std::move(shared_ptr);
            m_ptr = m_sharedPtr.get();
            return *this;
        }

        template<typename RT>
        shared_or_raw_ptr<T>& operator=(std::unique_ptr<RT> shared_ptr) noexcept
        {
            m_sharedPtr = std::move(shared_ptr);
            m_ptr = m_sharedPtr.get();
            return *this;
        }

        shared_or_raw_ptr<T>& operator=(T* raw_ptr) noexcept
        {
            m_sharedPtr.reset();
            m_ptr = raw_ptr;
            return *this;
        }

        [[nodiscard]] T* operator->() const noexcept { return m_ptr; }
        [[nodiscard]] T& operator*() const noexcept  { return *m_ptr; }

        [[nodiscard]] T* get() const noexcept { return m_ptr; }

        [[nodiscard]] explicit operator bool() const noexcept { return ( m_ptr != nullptr ); }

        template<typename RT>
        [[nodiscard]] bool operator==(const RT& rhs) const noexcept { return ( m_ptr == static_cast<const T*>(rhs) ); }

        template<typename RT>
        [[nodiscard]] bool operator!=(const RT& rhs) const noexcept { return ( m_ptr != static_cast<const T*>(rhs) ); }

    private:
        std::shared_ptr<T> m_sharedPtr;
        T* m_ptr;
    };


    
    // --------------------------------------------------------------------------
    // non_null_shared_or_raw_ptr
    // 
    //  - a subclass of shared_or_raw_ptr that checks that the pointer is not
    //    null on construction
    // --------------------------------------------------------------------------

#ifdef _DEBUG
    template<typename T>
    class non_null_shared_or_raw_ptr : public shared_or_raw_ptr<T>
    {
    public:
        template<typename RT>
        non_null_shared_or_raw_ptr(RT ptr) noexcept
            :   shared_or_raw_ptr<T>(std::move(ptr))
        {
            ASSERT(shared_or_raw_ptr<T>::get() != nullptr);
        }

        template<typename RT>
        non_null_shared_or_raw_ptr<T>& operator=(RT&& ptr) noexcept
        {
            shared_or_raw_ptr<T>::operator=(std::forward<RT>(ptr));
            ASSERT(shared_or_raw_ptr<T>::get() != nullptr);
            return *this;
        }
    };
#else
#define non_null_shared_or_raw_ptr shared_or_raw_ptr
#endif
}
