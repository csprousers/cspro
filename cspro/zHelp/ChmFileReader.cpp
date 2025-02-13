#include "stdafx.h"
#include "ChmFileReader.h"
#include <external/CHMLib/chm_lib.h>


ChmFileReader::ChmFileReader(const std::wstring& filename)
{
    m_chmHandle = chm_open(filename.c_str());

    if( m_chmHandle == nullptr )
        throw CSProException(_T("Unable to open CHM file: %s"), filename.c_str());

    m_defaultTopicPath = IdentifyDefaultTopicPath();
    ASSERT(!m_defaultTopicPath.empty());
}


ChmFileReader::ChmFileReader(ChmFileReader&& rhs) noexcept
    :   m_chmHandle(rhs.m_chmHandle),
        m_defaultTopicPath(std::move(rhs.m_defaultTopicPath))
{
    rhs.m_chmHandle = nullptr;
}


ChmFileReader::~ChmFileReader()
{
    if( m_chmHandle != nullptr )
        chm_close(m_chmHandle);
}


std::vector<std::byte> ChmFileReader::ReadObject(const std::string& object_path)
{
    // objects must begin with a / character
    if( !object_path.empty() && object_path.front() != '/' )
        return ReadObject("/" + object_path);

    chmUnitInfo chm_unit_info;

    if( chm_resolve_object(m_chmHandle, object_path.c_str(), &chm_unit_info) != CHM_RESOLVE_SUCCESS )
        throw CSProException("Unable to find: " + object_path);

    std::vector<std::byte> content(static_cast<size_t>(chm_unit_info.length));

    if( chm_retrieve_object(m_chmHandle, &chm_unit_info, reinterpret_cast<uint8_t*>(content.data()),
                            0, chm_unit_info.length) != static_cast<LONGINT64>(chm_unit_info.length) )
    {
        throw CSProException("Unable to read: " + object_path);
    }

    return content;
}


std::string ChmFileReader::IdentifyDefaultTopicPath()
{
    // modeled after SumatraPDF's ChmFile::ParseSystemData with the format from:
    // https://www.nongnu.org/chmspec/latest/Internal.html#SYSTEM

    std::vector<std::byte> system_data = ReadObject("/#SYSTEM");

    // parse the data, ignoring all but the default topic
    const std::byte* data = system_data.data();
    const std::byte* data_end = data + system_data.size();

    while( data < data_end )
    {
        auto check_length = [&](size_t length)
        {
            if( ( data + length ) > data_end )
                throw CSProException(_T("Unable to parse object of length %d"), (int)length);
        };

        auto get_word_le = [&]() -> WORD
        {
            check_length(2);
            return static_cast<uint8_t>(data[0]) | ( static_cast<uint8_t>(data[1]) << 8 );
        };

        WORD code = get_word_le();
        data += 2;

        WORD length = get_word_le();
        data += 2;

        check_length(length);

        if( code == 2 ) // Value of Default topic in the [OPTIONS] section of the HHP file. NT
        {
            // strings are null-terminated
            ASSERT(data[length - 1] == static_cast<std::byte>(0));
            return std::string(reinterpret_cast<const char*>(data), length - 1);
        }

        data += length;
    }

    throw CSProException("No default topic was identified");
}
