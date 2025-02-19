#include "StdAfx.h"
#include "TextSource.h"
#include <zToolsO/Serializer.h>


// --------------------------------------------------------------------------
// TextSource
// --------------------------------------------------------------------------

const std::string& TextSource::GetText() const
{
    return ReturnProgrammingError(SO::Empty_string);
}


SharableString TextSource::GetTextAsSharableString() const
{
    return ReturnProgrammingError(SharableString());
}


int64_t TextSource::GetModifiedIteration() const
{
    return ReturnProgrammingError(0);
}


void TextSource::SetText(SharableString /*text*/)
{
    ASSERT(false);
}


bool TextSource::RequiresSave() const
{
    return ReturnProgrammingError(false);
}


void TextSource::Save()
{
    ASSERT(false);
}


void TextSource::serialize(Serializer& ar)
{
    ar.SerializePath(m_filePath);
}
