#include "StdAfx.h"
#include "TextSourceExternal.h"
#include "ApplicationLoadException.h"


TextSourceExternal::TextSourceExternal(std::string file_path)
    :   TextSource(std::move(file_path))
{
    if( !PortableFunctions::FileIsRegular(m_filePath) )
        throw ApplicationFileNotFoundException(m_filePath);
}


void TextSourceExternal::SyncText() const
{
    // only load the file if it has changed
    const int64_t current_iteration = GetModifiedIteration();

    if( m_iterationAndText == nullptr || std::get<0>(*m_iterationAndText) != current_iteration )
    {
        try
        {
            m_iterationAndText = std::make_unique<std::tuple<int64_t, SharableString>>(current_iteration, FileIO::ReadText(m_filePath));
        }

        catch(...)
        {
            throw ApplicationFileLoadException(m_filePath);
        }
    }
}


const std::string& TextSourceExternal::GetText() const
{
    SyncText();
    return *std::get<1>(*m_iterationAndText);
}


SharableString TextSourceExternal::GetTextAsSharableString() const
{
    SyncText();
    return std::get<1>(*m_iterationAndText);
}


int64_t TextSourceExternal::GetModifiedIteration() const
{
    return PortableFunctions::FileModifiedTime(m_filePath);
}


bool TextSourceExternal::RequiresSave() const
{
    return false;
}
