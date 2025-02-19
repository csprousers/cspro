#include "StdAfx.h"
#include "IniFile.h"
#include <zUtilO/Versioning.h>


// --------------------------------------------------------------------------
// IniFileReader
// --------------------------------------------------------------------------

bool IniFileReader::ReadLine(std::string& attribute, std::string& value, const bool trim/* = true*/)
{
    // use an unread line (if available)
    if( !m_unreadLines.empty() )
    {
        std::tie(attribute, value) = std::move(m_unreadLines.back());
        m_unreadLines.pop_back();
        return true;
    }

    // read a new line
    if( !TextFile::ReadLine(attribute) )
        return false;

    ++m_linesRead;

    // lines may need to be combined
    while( !attribute.empty() && attribute.back() == ContinuationLineIndicator )
    {
        // use value to read in the continuation line
        if( !TextFile::ReadLine(value) )
            break;

        attribute.pop_back();
        attribute.append(value);

        ++m_linesRead;
    }

    if( trim )
        SO::MakeTrim(attribute);

    // split the line into the attribute and value
    const size_t separator_pos = attribute.find(AttributeValueSeparatorChar);

    if( separator_pos == std::string::npos )
    {
        value.clear();
    }

    else
    {
        value = attribute.substr(separator_pos + 1);
        attribute.resize(separator_pos);

        if( trim )
        {
            SO::MakeTrimRight(attribute);
            SO::MakeTrimLeft(value);
        }
    }

    return true;
}


void IniFileReader::UnreadLine(std::string attribute, std::string value)
{
    ASSERT(m_linesRead > 0);
    m_unreadLines.emplace_back(std::move(attribute), std::move(value));
}



// --------------------------------------------------------------------------
// IniFileWriter
// --------------------------------------------------------------------------

IniFileWriter& IniFileWriter::WriteVersion()
{
    return WriteLine(VersionKey, Versioning::CSProVersionText);
}
