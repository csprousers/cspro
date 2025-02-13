#pragma once


class HandleHolder
{
public:
    HandleHolder()
        :   m_handle(nullptr)
    {
    }

    explicit HandleHolder(HANDLE handle)
        :   m_handle(handle)
    {
    }

    HandleHolder(const HandleHolder&) = delete;

    HandleHolder(HandleHolder&& rhs) noexcept
    {
        *this = std::move(rhs);
    }

    ~HandleHolder()
    {
        if( m_handle != nullptr )
            CloseHandle(m_handle);
    }

    HandleHolder& operator=(const HandleHolder&) = delete;

    HandleHolder& operator=(HandleHolder&& rhs) noexcept
    {
        if( m_handle != rhs.m_handle )
        {
            m_handle = rhs.m_handle;
            rhs.m_handle = nullptr;
        }

        return *this;
    }

    operator HANDLE() const { return m_handle; }

    HANDLE* operator&() { return &m_handle; }

    HANDLE Release() { return std::exchange(m_handle, nullptr); }

private:
    HANDLE m_handle;
};
