#include "StdAfx.h"
#include "TextSourceExternal.h"
#include "ApplicationLoadException.h"


TextSourceExternal::TextSourceExternal(std::wstring filename)
    :   TextSource(std::move(filename))
{
    if( !PortableFunctions::FileIsRegular(m_filename) )
        throw ApplicationFileNotFoundException(m_filename);
}


const std::wstring& TextSourceExternal::GetText() const
{
    // only load the file if it has changed
    int64_t current_iteration = GetModifiedIteration();

    if( m_iterationAndText == nullptr || std::get<0>(*m_iterationAndText) != current_iteration )
    {
        try
        {
            m_iterationAndText = std::make_unique<std::tuple<int64_t, std::wstring>>(current_iteration, FileIO::ReadText(m_filename));
        }

        catch(...)
        {
            throw ApplicationFileLoadException(m_filename);
        }
    }

    return std::get<1>(*m_iterationAndText);
}


int64_t TextSourceExternal::GetModifiedIteration() const
{
    return PortableFunctions::FileModifiedTime(m_filename);
}
