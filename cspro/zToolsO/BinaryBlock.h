#pragma once


// --------------------------------------------------------------------------
// BinaryBlock
//
// BinaryBlock can be used to store data, using smart pointers, without
// using std::vector.
//
// Look for BINARY_BLOCK_TODO for some places that can be refactored to use
// BinaryBlock, particularly if we make this a base class for different
// storage strategies:
//
//     - std::shared_ptr<const std::vector<std::byte>>
//     - SharedString
//     - etc.
// --------------------------------------------------------------------------

class BinaryBlock
{
public:
    BinaryBlock(size_t size);

    template<typename T = std::byte>
    BinaryBlock(const T* data, size_t size);

    BinaryBlock(const BinaryBlock&) = delete;
    BinaryBlock(BinaryBlock&&) = default;

    BinaryBlock& operator=(const BinaryBlock&) = delete;
    BinaryBlock& operator=(BinaryBlock&&) = default;

    [[nodiscard]] bool empty() const { return ( m_size == 0 ); }

    [[nodiscard]] size_t size() const { return m_size; }

    // Returns the data pointer, defaulting to std::byte*,
    // but it can be cast to other single-byte types.
    template<typename T = std::byte>
    [[nodiscard]] const T* data() const;

    template<typename T = std::byte>
    [[nodiscard]] T* data();

    // Represents the data as another format, including std::string and std::string_view.
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
BinaryBlock::BinaryBlock(const T* const data, const size_t size)
    :   m_size(size),
        m_data(std::make_unique_for_overwrite<std::byte[]>(m_size))
{
    static_assert(sizeof(T) == sizeof(std::byte));
    memcpy(m_data.get(), data, size);
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
[[nodiscard]] inline std::string BinaryBlock::as() const
{
    return std::string(data<char>(), m_size);
}


template<>
[[nodiscard]] inline std::string_view BinaryBlock::as() const
{
    return std::string_view(data<char>(), m_size);
}
