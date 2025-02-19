#pragma once

#if __cplusplus < 202002L
// for std::make_unique_for_overwrite
#include <zToolsO/StandardTemplates.h>
#else
static_assert(false);
#endif


// BinaryBlock can be used to store data, using smart pointers, without using std::vector.

// look for BINARY_BLOCK_TODO for some places that can be refactored to use BinaryBlock, particularly
// if we make this a base class for different storage strategies:
//     - std::shared_ptr<const std::vector<std::byte>>
//     - SharedString
//     - etc.

class BinaryBlock
{
public:
    BinaryBlock(size_t size);

    BinaryBlock(const BinaryBlock&) = delete;
    BinaryBlock(BinaryBlock&&) = default;

    BinaryBlock& operator=(const BinaryBlock&) = delete;
    BinaryBlock& operator=(BinaryBlock&&) = default;

    [[nodiscard]] bool empty() const { return ( m_size == 0 ); }

    [[nodiscard]] size_t size() const { return m_size; }

    // returns the data pointer, defaulting to std::byte*, but it can be cast to other single-byte types
    template<typename T = std::byte>
    [[nodiscard]] const T* data() const;

    template<typename T = std::byte>
    [[nodiscard]] T* data();

    // represents the data as another format, including std::string_view
    template<typename T>
    [[nodiscard]] T as() const;

private:
    size_t m_size;
    std::unique_ptr<std::byte[]> m_data;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline BinaryBlock::BinaryBlock(size_t size)
    :   m_size(size),
        m_data(std::make_unique_for_overwrite<std::byte[]>(m_size))
{
}


template<typename T/* = std::byte*/>
[[nodiscard]] const T* BinaryBlock::data() const
{
    static_assert(sizeof(T) == sizeof(std::byte));
    return reinterpret_cast<const T*>(m_data.get());
}


template<typename T/* = std::byte*/>
[[nodiscard]] T* BinaryBlock::data()
{
    static_assert(sizeof(T) == sizeof(std::byte));
    return reinterpret_cast<T*>(m_data.get());
}


template<>
[[nodiscard]] inline std::string_view BinaryBlock::as() const
{
    return std::string_view(data<char>(), m_size);
}
