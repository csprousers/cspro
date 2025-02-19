#pragma once


class BinarySerializer
{
public:
    // returns the number of bytes needed to write a value of type T
    template<typename T>
    static constexpr size_t GetSize();

    // returns the number of bytes needed to write the specific value
    template<typename T>
    static size_t GetSize(const T& value);

    // returns the number of bytes needed to write length (size_t) values
    static constexpr size_t GetLengthSize();

    // writes the value to buffer (if non-null), returning how many bytes are (or would be) written
    template<typename T>
    static size_t Write(std::byte* buffer, const T& value);

    // writes a length (size_t) value to the non-null buffer
    static size_t WriteLength(std::byte* buffer, size_t value);

    // reads the value from the non-null buffer, advancing the buffer past the number of bytes read
    template<typename T>
    static void Read(const std::byte*& buffer, T& value);

    // reads and returns a value of type T from the non-null buffer, advancing the buffer past the number of bytes read
    template<typename T>
    static T Read(const std::byte*& buffer);

    // reads and returns a length (size_t) value from the non-null buffer, advancing the buffer past the number of bytes read
    static size_t ReadLength(const std::byte*& buffer);

private:
    using LengthType = uint64_t;

    template<typename T>
    static constexpr bool SupportedWithoutSpecialization();
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
constexpr bool BinarySerializer::SupportedWithoutSpecialization()
{
    if constexpr(std::is_same_v<T, bool> ||
                 std::is_same_v<T, int> ||
                 std::is_same_v<T, unsigned int> ||
                 std::is_same_v<T, int64_t> ||
                 std::is_same_v<T, uint64_t> ||
                 std::is_same_v<T, double>)
    {
        static_assert(sizeof(bool) == 1);
        static_assert(sizeof(int) == 4);
        static_assert(sizeof(unsigned int) == 4);
        static_assert(sizeof(int64_t) == 8);
        static_assert(sizeof(uint64_t) == 8);
        static_assert(sizeof(double) == 8);

        return true;
    }

    else
    {
        return false;
    }
}


template<typename T>
constexpr size_t BinarySerializer::GetSize()
{
    if constexpr(SupportedWithoutSpecialization<T>())
    {
        return sizeof(T);
    }

    else
    {
        static_assert_false();
    }
}


template<typename T>
size_t BinarySerializer::GetSize(const T& value)
{
    if constexpr(std::is_same_v<T, std::string_view> ||
                 std::is_same_v<T, std::string>)
    {
        return GetLengthSize() + value.length();
    }

    else if constexpr(std::is_same_v<T, std::vector<std::byte>>)
    {
        return GetLengthSize() + value.size();
    }

    else
    {
        return GetSize<T>();
    }
}


constexpr size_t BinarySerializer::GetLengthSize()
{
    return GetSize<LengthType>();
}


template<typename T>
size_t BinarySerializer::Write(std::byte* const buffer, const T& value)
{
    static_assert(SupportedWithoutSpecialization<T>() && GetSize<T>() == sizeof(T));

    if( buffer != nullptr )
        memcpy(buffer, &value, sizeof(T));

    return sizeof(T);
}


template<>
inline size_t BinarySerializer::Write(std::byte* const buffer, const std::string_view& value_sv)
{
    const size_t size_length = WriteLength(buffer, value_sv.length());

    if( buffer != nullptr )
        memcpy(buffer + size_length, value_sv.data(), value_sv.length());

    return size_length + value_sv.length();
}


template<>
inline size_t BinarySerializer::Write(std::byte* const buffer, const std::string& value)
{
    return Write(buffer, std::string_view(value));
}


template<>
inline size_t BinarySerializer::Write(std::byte* const buffer, const std::vector<std::byte>& value)
{
    const size_t size_length = WriteLength(buffer, value.size());

    if( buffer != nullptr )
        memcpy(buffer + size_length, value.data(), value.size());

    return size_length + value.size();
}


inline size_t BinarySerializer::WriteLength(std::byte* const buffer, const size_t value)
{
    return Write(buffer, static_cast<LengthType>(value));
}


template<typename T>
void BinarySerializer::Read(const std::byte*& buffer, T& value)
{
    static_assert(SupportedWithoutSpecialization<T>() && GetSize<T>() == sizeof(T));
    memcpy(&value, buffer, sizeof(T));
    buffer += sizeof(T);
}


template<>
inline void BinarySerializer::Read(const std::byte*& buffer, std::string& value)
{
    const size_t string_length = ReadLength(buffer);
    value.assign(reinterpret_cast<const char*>(buffer), string_length);
    buffer += string_length;
}


template<>
inline void BinarySerializer::Read(const std::byte*& buffer, std::vector<std::byte>& value)
{
    const size_t vector_length = ReadLength(buffer);
    value.assign(buffer, buffer + vector_length);
    buffer += vector_length;
}


template<typename T>
T BinarySerializer::Read(const std::byte*& buffer)
{
    T value;
    Read(buffer, value);
    return value;
}


template<>
inline std::string BinarySerializer::Read(const std::byte*& buffer)
{
    const size_t string_length = ReadLength(buffer);
    std::string value(reinterpret_cast<const char*>(buffer), string_length);
    buffer += string_length;
    return value;
}


inline size_t BinarySerializer::ReadLength(const std::byte*& buffer)
{
    return static_cast<size_t>(Read<LengthType>(buffer));
}
