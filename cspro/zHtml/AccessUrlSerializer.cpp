#include "stdafx.h"
#include "AccessUrlSerializer.h"
#include "PortableLocalhost.h"


void AccessUrl::SerializerHelper::WriteFileAccessUrl(JsonWriter& json_writer, NullTerminatedStringView filename)
{
    if( PortableFunctions::FileIsRegular(filename) )
        json_writer.Write(JK::url, PortableLocalhost::CreateFilenameUrl(filename));
}
