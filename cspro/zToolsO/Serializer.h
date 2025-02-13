#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/CSProException.h>
#include <zToolsO/SerializerHelper.h>
#include <zUtilO/ApplicationLoadException.h>

template<typename T> class ConstantConserver;


struct SerializationException : public ApplicationLoadException
{
    using ApplicationLoadException::ApplicationLoadException;
};


class SerializerImpl
{
public:
    virtual ~SerializerImpl() { }

    virtual void Open(const std::wstring& filename) = 0;
    virtual void Create(const std::wstring& filename) = 0;
    virtual void Close() = 0;
    virtual void Read(void* buffer, int length) = 0;
    virtual void Write(const void* buffer, int length) = 0;
};


class CLASS_DECL_ZTOOLSO Serializer
{
public:
    // released version numbers (_x_x_xxx) and then an iteration number (_x) for betas
    static constexpr int Iteration_7_5_000_1 = 750001;

    static constexpr int Iteration_7_6_000_1 = 760001;

    static constexpr int Iteration_7_7_000_1 = 770001;
    static constexpr int Iteration_7_7_000_2 = 770002;

    static constexpr int Iteration_8_0_000_1 = 800001;
    static constexpr int Iteration_8_0_000_2 = 800002;
    static constexpr int Iteration_8_0_000_3 = 800003;
    static constexpr int Iteration_8_0_000_4 = 800004;
    static constexpr int Iteration_8_0_002_1 = 800021;

    static constexpr int GetEarliestSupportedVersion() { return Iteration_7_5_000_1; }

    static constexpr int GetCurrentVersion()           { return Iteration_8_0_002_1; }

public:
    Serializer();
    ~Serializer();

    void OpenInputArchive(std::wstring filename);
    void CreateOutputArchive(std::wstring filename, bool use_string_conserver = true);
    void CloseArchive();

    bool IsSaving() const  { return m_saving; }
    bool IsLoading() const { return !m_saving; }

    int GetArchiveVersion() const                      { return m_archiveVersion; }
    bool MeetsVersionIteration(int iteration) const    { return ( m_archiveVersion >= iteration ); }
    bool PredatesVersionIteration(int iteration) const { return ( m_archiveVersion < iteration ); }

    const std::wstring& GetArchiveFilename() const { return m_archiveFilename; }
    time_t GetArchiveModifiedDate() const          { return m_archiveModifiedDate; }

    void Read(void* buffer, int length)        { m_serializerImpl->Read(buffer, length); }
    void Write(const void* buffer, int length) { m_serializerImpl->Write(buffer, length); }

    SerializerHelper& GetSerializerHelper() { return m_serializerHelper; }

private:
    std::unique_ptr<SerializerImpl> m_serializerImpl;
    SerializerHelper m_serializerHelper;
    std::wstring m_archiveFilename;
    time_t m_archiveModifiedDate;
    int m_archiveVersion;
    bool m_saving;

    std::vector<std::wstring> m_serializedStrings;
    std::unique_ptr<ConstantConserver<std::wstring>> m_serializedStringConserver;


public:
    void WriteFilename(NullTerminatedString filename);
    Serializer& SerializeFilename(CString& filename, bool normalize_filename = false);
    Serializer& SerializeFilename(std::wstring& filename, bool normalize_filename = false);
    Serializer& SerializeFilenameArray(std::vector<CString>& filenames, bool normalize_filename = false);

    template<typename T>
    void SerializeString(T& str);

    void Dump(void* buffer, int length)
    {
        if( m_saving )
        {
            Write(buffer, length);
        }

        else
        {
            Read(buffer, length);
        }
    }


    template<typename T>
    Serializer& operator&(T& value)
    {
        if( m_saving )
        {
            *this << value;
        }

        else
        {
            *this >> value;
        }

        return *this;
    }

    template<typename T>
    Serializer& operator<<(const T& value)
    {
        serialize(*this, const_cast<T&>(value));
        return *this;
    }

    template<typename T>
    Serializer& operator>>(T& value)
    {
        serialize(*this, value);
        return *this;
    }


#define SerializeBasicType(ValueType)                                                                  \
    Serializer& operator<<(const ValueType& value) { Write(&value, sizeof(ValueType)); return *this; } \
    Serializer& operator>>(ValueType& value)       { Read(&value, sizeof(ValueType));  return *this; }

    SerializeBasicType(bool)

    SerializeBasicType(char)
    SerializeBasicType(unsigned char)

    SerializeBasicType(wchar_t)

    SerializeBasicType(short)
    SerializeBasicType(unsigned short)

    SerializeBasicType(int)
    SerializeBasicType(unsigned int)

#ifndef ANDROID
    // because long on Android will be eight bytes in 64-bit,
    // these are not included because anything that was serialized
    // to a long on Windows should be typedefed to an int on Android
    SerializeBasicType(long)
    SerializeBasicType(unsigned long)
#endif

    SerializeBasicType(float)
    SerializeBasicType(double)
    SerializeBasicType(long double)

#undef SerializeBasicType


    template<typename T>
    Serializer& SerializeEnum(T& enum_value);

    template<typename T>
    Serializer& SerializeEnum(std::optional<T>& enum_value);

    Serializer& operator<<(const std::string& value);
    Serializer& operator>>(std::string& value);

    template<typename T>
    T Read()
    {
        T value;
        *this >> value;
        return value;
    }

    template<typename T>
    void Write(const T& value)
    {
        *this << value;
    }

    template<typename T>
    Serializer& IgnoreUnusedVariable(int first_unused_iteration)
    {
        if( IsLoading() && PredatesVersionIteration(first_unused_iteration) )
            Read<T>();

        return *this;
    }
};


template<typename T>
static Serializer& serialize(Serializer& ar, T& object)
{
    object.serialize(ar);
    return ar;
}


CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, std::wstring& str);
CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, CString& str);
CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, size_t& value);

CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, CPoint& rect);
CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, CRect& rect);
CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, LOGFONT& font);
CLASS_DECL_ZTOOLSO Serializer& serialize(Serializer& ar, POINT& point);



// --------------------------------------------------------------------------
// serializing std::optional objects
// --------------------------------------------------------------------------

template<typename T, typename SerializerFunction>
static Serializer& serialize(Serializer& ar, std::optional<T>& optional_value, SerializerFunction serializer_function)
{
    bool serialize_value;

    if( ar.IsSaving() )
        serialize_value = optional_value.has_value();

    ar & serialize_value;

    if( ar.IsLoading() )
    {
        ASSERT(!optional_value.has_value());

        if( serialize_value )
            optional_value = T();
    }

    if( serialize_value )
        serializer_function(*optional_value);

    return ar;
}

static Serializer& serialize(Serializer& ar, std::optional<int>& optional_value)
{
    return serialize(ar, optional_value, [&](int& value) { ar & value; });
}

static Serializer& serialize(Serializer& ar, std::optional<size_t>& optional_value)
{
    return serialize(ar, optional_value, [&](size_t& value) { ar & value; });
}

static Serializer& serialize(Serializer& ar, std::optional<double>& optional_value)
{
    return serialize(ar, optional_value, [&](double& value) { ar & value; });
}



// --------------------------------------------------------------------------
// serializing std::vector objects
// --------------------------------------------------------------------------

template<typename T, typename SerializerFunction>
static Serializer& vector_serialize(Serializer& ar, std::vector<T>& vector_of_values, SerializerFunction serializer_function)
{
    if( ar.IsSaving() )
    {
        ar.Write<size_t>(vector_of_values.size());
    }

    else
    {
        vector_of_values.resize(ar.Read<size_t>());
    }

    for( auto& value : vector_of_values )
        serializer_function(value);

    return ar;
}

template<typename T>
static Serializer& serialize(Serializer& ar, std::vector<T>& vector_of_values)
{
    return vector_serialize(ar, vector_of_values,
        [&](T& value)
        {
            ar & value;
        });
}

template<typename T>
static Serializer& serialize_enum_vector(Serializer& ar, std::vector<T>& vector_of_enums)
{
    return vector_serialize(ar, vector_of_enums,
        [&](T& value)
        {
            ar.SerializeEnum(value);
        });
}

template<typename T>
static Serializer& serialize_optional_enum_vector(Serializer& ar, std::vector<std::optional<T>>& vector_of_optional_enums)
{
    return vector_serialize(ar, vector_of_optional_enums,
        [&](std::optional<T>& optional_value)
        {
            serialize(ar, optional_value,
                [&](T& value)
                {
                    ar.SerializeEnum(value);
                });
        });
}

template<typename T>
static Serializer& serialize(Serializer& ar, std::vector<T*>& vector_of_pointers)
{
    return vector_serialize(ar, vector_of_pointers,
        [&](T*& value_pointer)
        {
            if( ar.IsLoading() )
                value_pointer = new T;

            ar & *value_pointer;
        });
}

template<typename T>
static Serializer& serialize(Serializer& ar, std::vector<std::shared_ptr<T>>& vector_of_shared_pointers)
{
    return vector_serialize(ar, vector_of_shared_pointers,
        [&](std::shared_ptr<T>& value_pointer)
        {
            if( ar.IsLoading() )
                value_pointer = std::make_shared<T>();

            ar & *value_pointer;
        });
}



// --------------------------------------------------------------------------
// serializing std::map objects
// --------------------------------------------------------------------------

template<class Key, class Value, typename SerializerFunction>
static Serializer& map_serialize(Serializer& ar, std::map<Key, Value>& map_of_values, SerializerFunction serializer_function)
{
    if( ar.IsSaving() )
    {
        ar.Write<size_t>(map_of_values.size());

        for( const auto& [key, value] : map_of_values )
            serializer_function(const_cast<Key&>(key), const_cast<Value&>(value));
    }

    else
    {
        for( size_t i = ar.Read<size_t>(); i > 0; --i )
        {
            Key key;
            Value value;
            serializer_function(key, value);
            map_of_values[key] = value;
        }
    }

    return ar;
}

template<class Key, class Value>
static Serializer& serialize(Serializer& ar, std::map<Key, Value>& map_of_values)
{
    return map_serialize(ar, map_of_values,
        [&](Key& key, Value& value)
        {
            ar & key
               & value;
        });
}



// --------------------------------------------------------------------------
// serializing std::set objects
// --------------------------------------------------------------------------

template<class Value>
static Serializer& serialize(Serializer& ar, std::set<Value>& set_of_values)
{
    if( ar.IsSaving() )
    {
        ar.Write<size_t>(set_of_values.size());

        for( const auto& value : set_of_values )
            ar << value;
    }

    else
    {
        for( size_t i = ar.Read<size_t>(); i > 0; --i )
            set_of_values.insert(ar.Read<Value>());
    }

    return ar;
}



// --------------------------------------------------------------------------
// serializing enums
// --------------------------------------------------------------------------

template<typename T>
Serializer& Serializer::SerializeEnum(T& enum_value)
{
    static_assert(sizeof(T) == sizeof(int));
    Dump(&enum_value, sizeof(int));
    return *this;
}


template<typename T>
Serializer& Serializer::SerializeEnum(std::optional<T>& enum_value)
{
    serialize(*this, enum_value,
        [&](T& value)
        {
            SerializeEnum(value);
        });

    return *this;
}



// --------------------------------------------------------------------------
// serializing smart pointers
// --------------------------------------------------------------------------

template<class T>
static Serializer& serialize(Serializer& ar, std::unique_ptr<T>& unique_ptr_value)
{
    if( ar.IsSaving() )
    {
        ar.Write<bool>(( unique_ptr_value != nullptr ));
    }

    else
    {
        ASSERT(unique_ptr_value == nullptr);

        if( ar.Read<bool>() )
            unique_ptr_value = std::make_unique<T>();
    }

    if( unique_ptr_value != nullptr )
        ar & *unique_ptr_value;

    return ar;
}



CLASS_DECL_ZTOOLSO void APP_LOAD_TODO_SetArchive(std::shared_ptr<Serializer> serializer);
CLASS_DECL_ZTOOLSO Serializer& APP_LOAD_TODO_GetArchive();
