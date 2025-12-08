#include "StdAfx.h"
#include "Serializer.h"
#include "ConstantConserver.h"
#include "PenSerializer.h"


// for reference, the following are the old serialization versions:
//
//   - Iteration_6_1_000_1 = 610001;
//   - Iteration_6_1_000_2 = 610002;
//   - Iteration_6_1_000_3 = 610003;
//
//   - Iteration_6_2_000_1 = 620001;
//   - Iteration_6_2_000_2 = 620002;
//   - Iteration_6_2_000_3 = 620003;
//
//   - Iteration_6_3_000_1 = 700001; // this number was actually released as 6.3
//
//   - Iteration_7_0_000_2 = 700002;
//   - Iteration_7_0_000_3 = 700003;
//   - Iteration_7_0_000_4 = 700004;
//   - Iteration_7_0_000_5 = 700005;
//
//   - Iteration_7_1_000_1 = 710001;
//   - Iteration_7_1_000_2 = 710002;
//   - Iteration_7_1_000_3 = 710003;
//   - Iteration_7_1_000_4 = 710004;
//   - Iteration_7_1_000_5 = 710005;
//   - Iteration_7_1_000_6 = 710006;
//
//   - Iteration_7_2_000_1 = 720001;
//
//   - Iteration_7_3_000_1 = 730001;
//   - Iteration_7_3_000_2 = 730002;
//
//   - Iteration_7_4_000_1 = 740001;
//   - Iteration_7_4_000_2 = 740002;
//   - Iteration_7_4_000_3 = 740003;
//   - Iteration_7_4_000_4 = 740004;
//
//   - Iteration_7_5_000_1 = 750001;
//
//   - Iteration_7_6_000_1 = 760001;


namespace
{
    // APP_LOAD_TODO remove
    std::shared_ptr<Serializer> APP_LOAD_TODO_instance;
}

void APP_LOAD_TODO_SetArchive(std::shared_ptr<Serializer> serializer)
{
    APP_LOAD_TODO_instance = std::move(serializer);
}

Serializer& APP_LOAD_TODO_GetArchive()
{
    if( APP_LOAD_TODO_instance == nullptr )
        throw ProgrammingErrorException();

    return *APP_LOAD_TODO_instance;
}



Serializer::Serializer()
    :   m_archiveModifiedDate(0),
        m_archiveVersion(0),
        m_saving(false)
{
}


Serializer::~Serializer()
{
    try
    {
        CloseArchive();
    }
    catch( const SerializationException& ) { ASSERT(false); }
}


void Serializer::OpenInputArchive(std::string file_path)
{
    m_archiveFilePath = std::move(file_path);
    m_archiveModifiedDate = PortableFunctions::FileModifiedTime(m_archiveFilePath);
    m_saving = false;

    m_serializerImpl = std::make_unique<PenSerializer>();
    m_serializerImpl->Open(m_archiveFilePath);

    *this >> m_archiveVersion;

    const bool use_string_conserver = Read<bool>();

    if( use_string_conserver )
        m_serializedStringConserver = std::make_unique<ConstantConserver<std::string>>(m_serializedStrings);
}


void Serializer::CreateOutputArchive(std::string file_path, const bool use_string_conserver/* = true*/)
{
    m_archiveFilePath = MakeFullPath(GetWorkingDirectory(), std::move(file_path));
    m_saving = true;

    m_archiveVersion = GetCurrentVersion();

    m_serializerImpl = std::make_unique<PenSerializer>();
    m_serializerImpl->Create(m_archiveFilePath);

    *this << m_archiveVersion
          << use_string_conserver;

    if( use_string_conserver )
        m_serializedStringConserver = std::make_unique<ConstantConserver<std::string>>(m_serializedStrings);
}


void Serializer::CloseArchive()
{
    if( m_serializerImpl != nullptr )
    {
        m_serializerImpl->Close();
        m_serializerImpl.reset();
    }
}


Serializer& Serializer::IgnoreUnusedBytes(int length)
{
    ASSERT(length > 0);

    if( IsLoading() )
    {
        constexpr int MaxBufferSize = 1024 * 1024;
        const int buffer_size = std::min(length, MaxBufferSize);
        auto buffer = std::make_unique_for_overwrite<char[]>(buffer_size);

        while( true )
        {
            if( length <= buffer_size )
            {
                Read(buffer.get(), length);
                break;
            }

            else
            {
                Read(buffer.get(), buffer_size);
                length -= buffer_size;
            }
        }
    }

    return *this;
}


// these path-related serialization functions were added so that the full path of various files aren't written out (starting with CSPro 6.1);
// previously, writing out the full path made the .pen file different if the application files were stored in different folders;
// e.g., creating a .pen file on two different computers resulted in different .pen files

void Serializer::WritePath(const std::string& path)
{
    ASSERT(IsSaving());
    *this << GetRelativeFName(UTF8_TODO::GetWide(m_archiveFilePath), UTF8_TODO::GetWide(path));
}


void Serializer::WriteFilename(NullTerminatedString filename)
{
    ASSERT(IsSaving());
    *this << GetRelativeFName(UTF8_TODO::GetWide(m_archiveFilePath), filename);
}


Serializer& Serializer::SerializePath(std::string& path, const bool adjust_to_use_native_slash/* = false*/)
{
    if( IsLoading() )
    {
        path = MakeFullPath(PortableFunctions::PathGetDirectory(m_archiveFilePath), Read<std::string>());

        if( adjust_to_use_native_slash )
            PortableFunctions::MakePathToNativeSlash(path);
    }

    else
    {
        WritePath(path);
    }

    return *this;
}


Serializer& Serializer::SerializePaths(std::vector<std::string>& paths, const bool adjust_to_use_native_slash/* = false*/)
{
    vector_serialize(*this, paths,
        [&](std::string& path)
        {
            SerializePath(path, adjust_to_use_native_slash);
        });

    return *this;
}


Serializer& Serializer::SerializeFilename(CString& filename, bool normalize_filename/* = false*/)
{
    if( IsLoading() )
    {
        std::string path;
        SerializePath(path, normalize_filename);
        filename = UTF8_TODO::GetCString(path);
    }

    else
    {
        WriteFilename(filename);
    }

    return *this;
}

Serializer& Serializer::SerializeFilename(std::wstring& filename, bool normalize_filename/* = false*/)
{
    if( IsLoading() )
    {
        std::string path;
        SerializePath(path, normalize_filename);
        filename = UTF8_TODO::GetWide(path);
    }

    else
    {
        WriteFilename(filename);
    }

    return *this;
}


// 20121108 routines for the new serialization, followed by the previous code

std::wstring Serializer::ReadPre81WideString()
{
    constexpr size_t SerializedCharacterLength = 2;

#ifdef WIN32
    static_assert(sizeof(wchar_t) == SerializedCharacterLength);
#endif

    int string_length = Read<int>();
    std::wstring value;

#ifdef WIN32
    value.resize(string_length);
    Read(value.data(), string_length * SerializedCharacterLength);

#else
    // read the string into an array of two-byte characters
    auto two_byte_string = std::make_unique_for_overwrite<uint16_t[]>(string_length);
    static_assert(sizeof(*two_byte_string.get()) == SerializedCharacterLength);

    Read(two_byte_string.get(), string_length * SerializedCharacterLength);

    // convert to wide characters
    value = TwoByteCharToWide(two_byte_string.get(), string_length);
#endif

    return value;
}


Serializer& Serializer::operator<<(const std::string& value)
{
    ASSERT(IsSaving());

    if( m_serializedStringConserver != nullptr )
    {
        const int string_index = m_serializedStringConserver->Add(value);
        const bool string_is_new = ( static_cast<size_t>(string_index + 1) == m_serializedStrings.size() );

        *this << string_is_new;

        if( !string_is_new )
        {
            *this << string_index;
            return *this;
        }
    }

    Write(value.length());
    Write(value.data(), value.length());

    return *this;
}


Serializer& Serializer::operator>>(std::string& value)
{
    ASSERT(IsLoading());

    if( m_serializedStringConserver != nullptr )
    {
        const bool string_is_new = Read<bool>();

        if( !string_is_new )
        {
            const size_t string_index = Read<int>();

            if( string_index >= m_serializedStrings.size() )
            {
                throw SerializationException("String conserver index not valid: %d requested with only %d entries",
                                             static_cast<int>(string_index), static_cast<int>(m_serializedStrings.size()));
            }

            value = m_serializedStrings[string_index];

            return *this;
        }
    }

    if( MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        const size_t string_length = Read<size_t>();
        value.resize(string_length);
        Read(value.data(), string_length);
    }

    else
    {
        value = TC::ToUtf8(ReadPre81WideString());
    }

    if( m_serializedStringConserver != nullptr )
        m_serializedStringConserver->Add(value);

    return *this;
}


Serializer& Serializer::operator<<(const SharableString& value)
{
    if( value.IsSet() )
    {
        *this << true << *value;
    }

    else
    {
        *this << false;
    }

    return *this;
}


Serializer& Serializer::operator>>(SharableString& value)
{
    if( MeetsVersionIteration(Serializer::Iteration_8_1_000_1) ? Read<bool>() : true )
    {
        *this >> value.MakeModifiable();
    }

    else
    {
        value.Reset();
    }

    return *this;
}


Serializer& serialize(Serializer& ar, size_t& value)
{
    if( ar.IsSaving() )
    {
#if defined(WIN_DESKTOP) && !defined(_WIN64)
        static_assert(sizeof(size_t) == sizeof(unsigned int));
#endif
        ar.Write<unsigned int>(uint32_cast(value));
    }

    else
    {
        value = ar.Read<unsigned int>();
    }

    return ar;
}


Serializer& serialize(Serializer& ar, CPoint& point)
{
    ar.Dump(&point, sizeof(point));
    return ar;
}


Serializer& serialize(Serializer& ar, CRect& rect)
{
    ar.Dump(&rect, sizeof(rect));
    return ar;
}


Serializer& serialize(Serializer& ar, LOGFONT& font)
{
    ar.Dump(&font, sizeof(font));
    return ar;
}


Serializer& serialize(Serializer& ar, POINT& point)
{
    ar & point.x
       & point.y;

    return ar;
}
