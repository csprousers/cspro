#include "stdafx.h"
#include "AccessUrlSerializer.h"
#include "PortableLocalhost.h"


void AccessUrl::SerializerHelper::WriteFileAccessUrl(JsonWriter& json_writer, InterfaceString file_path)
{
    if( PortableFunctions::FileIsRegular(file_path) )
        json_writer.Write(JK::url, PortableLocalhost::CreateFileUrl(file_path.GetString<std::string>()));
}
