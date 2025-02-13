#include "stdafx.h"
#include "Main.h"
#include <zToolsO/Serializer.h>


void AssetsGenerator::Create()
{
    MessageFile message_file;
    MessageLoader::LoadMessageFiles(message_file, false, nullptr);

    Serializer serializer;
    serializer.CreateOutputArchive(L"system.mgf", false);
    serializer >> message_file;
    serializer.CloseArchive();
}
