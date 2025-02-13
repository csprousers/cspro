#include "StdAfx.h"
#include "TextSource.h"
#include <zToolsO/Serializer.h>


void TextSource::serialize(Serializer& ar)
{
    ar.SerializeFilename(m_filename);
}


void NamedTextSource::serialize(Serializer& ar)
{
    ASSERT(ar.IsLoading() == ( text_source == nullptr ));

    if( ar.IsLoading() )
        text_source = std::make_shared<TextSource>();

    ar & name
       & *text_source;
}
