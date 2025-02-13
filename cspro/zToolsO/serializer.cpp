#include "StdAfx.h"
#include "Serializer.h"
#include "ConstantConserver.h"
#include "PenSerializer.h"


// for reference, the following are the old serialization versions
/*
    static const int Iteration_6_1_000_1 = 610001;
    static const int Iteration_6_1_000_2 = 610002;
    static const int Iteration_6_1_000_3 = 610003;

    static const int Iteration_6_2_000_1 = 620001;
    static const int Iteration_6_2_000_2 = 620002;
    static const int Iteration_6_2_000_3 = 620003;

    static const int Iteration_6_3_000_1 = 700001; // this number was actually released as 6.3

    static const int Iteration_7_0_000_2 = 700002;
    static const int Iteration_7_0_000_3 = 700003;
    static const int Iteration_7_0_000_4 = 700004;
    static const int Iteration_7_0_000_5 = 700005;

    static const int Iteration_7_1_000_1 = 710001;
    static const int Iteration_7_1_000_2 = 710002;
    static const int Iteration_7_1_000_3 = 710003;
    static const int Iteration_7_1_000_4 = 710004;
    static const int Iteration_7_1_000_5 = 710005;
    static const int Iteration_7_1_000_6 = 710006;

    static const int Iteration_7_2_000_1 = 720001;

    static const int Iteration_7_3_000_1 = 730001;
    static const int Iteration_7_3_000_2 = 730002;

    static const int Iteration_7_4_000_1 = 740001;
    static const int Iteration_7_4_000_2 = 740002;
    static const int Iteration_7_4_000_3 = 740003;
    static const int Iteration_7_4_000_4 = 740004;

    when only supporting [] or greater...

        Iteration_7_6_000_1:
            - remove uses of alpha/string that used VART instead of WorkString/WorkAlpha
                [so look for uses of VART with subtype SymbolSubType::Work]
            - remove the Workdict object
*/


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
    catch( const SerializationException& ) { }
}


void Serializer::OpenInputArchive(std::wstring filename)
{
    m_archiveFilename = std::move(filename);
    m_archiveModifiedDate = PortableFunctions::FileModifiedTime(m_archiveFilename);
    m_saving = false;

    m_serializerImpl = std::make_unique<PenSerializer>();
    m_serializerImpl->Open(m_archiveFilename);

    *this >> m_archiveVersion;

    bool use_string_conserver = Read<bool>();

    if( use_string_conserver )
        m_serializedStringConserver = std::make_unique<ConstantConserver<std::wstring>>(m_serializedStrings);
}


void Serializer::CreateOutputArchive(std::wstring filename, bool use_string_conserver/* = true*/)
{
    m_archiveFilename = MakeFullPath(GetWorkingFolder(), std::move(filename));
    m_saving = true;

    m_archiveVersion = GetCurrentVersion();

    m_serializerImpl = std::make_unique<PenSerializer>();
    m_serializerImpl->Create(m_archiveFilename);

    *this << m_archiveVersion
          << use_string_conserver;

    if( use_string_conserver )
        m_serializedStringConserver = std::make_unique<ConstantConserver<std::wstring>>(m_serializedStrings);
}


void Serializer::CloseArchive()
{
    if( m_serializerImpl != nullptr )
    {
        m_serializerImpl->Close();
        m_serializerImpl.reset();
    }
}


// these filename serialization functions were added so that the full path filenames of various files aren't written out (starting with CSPro 6.1)
// previously, writing out the full path made the .pen file different if the application files were stored in different folders;
// e.g., creating a .pen file on two different computers resulted in different .pen files
void Serializer::WriteFilename(NullTerminatedString filename)
{
    ASSERT(IsSaving());
    *this << GetRelativeFName(m_archiveFilename, filename);
}


Serializer& Serializer::SerializeFilename(CString& filename, bool normalize_filename/* = false*/)
{
    if( IsLoading() )
    {
        filename = WS2CS(MakeFullPath(PortableFunctions::PathGetDirectory(m_archiveFilename), Read<std::wstring>()));

        if( normalize_filename )
            NormalizePathSlash(filename);
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
        filename = MakeFullPath(PortableFunctions::PathGetDirectory(m_archiveFilename), Read<std::wstring>());

        if( normalize_filename )
            NormalizePathSlash(filename);
    }

    else
    {
        WriteFilename(filename);
    }

    return *this;
}


Serializer& Serializer::SerializeFilenameArray(std::vector<CString>& filenames, bool normalize_filename/* = false*/)
{
    vector_serialize(*this, filenames,
        [&](CString& filename)
        {
            SerializeFilename(filename, normalize_filename);
        });

    return *this;
}


// 20121108 routines for the new serialization, followed by the previous code

template<typename T>
void Serializer::SerializeString(T& str)
{
    constexpr size_t SerializedCharacterLength = 2;

#ifdef WIN32
    static_assert(sizeof(TCHAR) == SerializedCharacterLength);
#endif

    if( IsSaving() )
    {
        wstring_view str_view = str;

        if( m_serializedStringConserver != nullptr )
        {
            int string_index = m_serializedStringConserver->Add(str_view);
            bool string_is_new = ( ( string_index + 1 ) == (int)m_serializedStrings.size() );

            *this << string_is_new;

            if( !string_is_new )
            {
                *this << string_index;
                return;
            }
        }

        Write<int>(str_view.length());
        Write(str_view.data(), str_view.length() * SerializedCharacterLength);
    }

    else
    {
        if( m_serializedStringConserver != nullptr ) // version 7.2+
        {
            bool string_is_new = Read<bool>();

            if( !string_is_new )
            {
                size_t string_index = Read<int>();

                if( string_index >= m_serializedStrings.size() )
                    throw SerializationException("String conserver index not valid");

                if constexpr(std::is_same_v<T, CString>)
                {
                    str = WS2CS(m_serializedStrings[string_index]);
                }

                else
                {
                    str = m_serializedStrings[string_index];
                }

                return;
            }
        }

        // read the string
        int string_length = Read<int>();

#ifdef WIN32
        if constexpr(std::is_same_v<T, CString>)
        {
            Read(str.GetBufferSetLength(string_length), string_length * SerializedCharacterLength);
            str.ReleaseBuffer();
        }

        else
        {
            str.resize(string_length);
            Read(str.data(), string_length * SerializedCharacterLength);
        }

#else
        // read the string into an array of two-byte characters
        auto two_byte_string = std::make_unique_for_overwrite<uint16_t[]>(string_length);
        static_assert(sizeof(*two_byte_string.get()) == SerializedCharacterLength);

        Read(two_byte_string.get(), string_length * SerializedCharacterLength);

        // conver to wide characters
        str = TwoByteCharToWide(two_byte_string.get(), string_length);
#endif

        if( m_serializedStringConserver != nullptr )
        {
            if constexpr(std::is_same_v<T, CString>)
            {
                m_serializedStringConserver->Add(CS2WS(str));
            }

            else
            {
                m_serializedStringConserver->Add(str);
            }
        }            
    }
}

Serializer& serialize(Serializer& ar, std::wstring& str)
{
    ar.SerializeString(str);
    return ar;
}

Serializer& serialize(Serializer& ar, CString& str)
{
    ar.SerializeString(str);
    return ar;
}


Serializer& serialize(Serializer& ar, size_t& value)
{
    if( ar.IsSaving() )
    {
#ifdef WIN_DESKTOP
        static_assert(sizeof(size_t) == sizeof(unsigned));
#endif
        ar.Write<unsigned>(value);
    }

    else
    {
        value = (size_t)ar.Read<unsigned>();
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


Serializer& Serializer::operator<<(const std::string& value)
{
    Write<int>(value.size());
    Write(value.data(), value.size());
    return *this;
}

Serializer& Serializer::operator>>(std::string& value)
{
    int string_length = 0;
    *this >> string_length;
    value.resize(string_length);
    Read(value.data(), string_length);
    return *this;
}
