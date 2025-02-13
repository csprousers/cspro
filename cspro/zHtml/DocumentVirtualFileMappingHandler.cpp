#include "stdafx.h"
#include "DocumentVirtualFileMappingHandler.h"
#include <zToolsO/FileIO.h>


DocumentVirtualFileMappingHandler::DocumentVirtualFileMappingHandler(const CDocument& document)
    :   m_document(document),
        m_filePath(GetFilePathForDocument(m_document))
{
    RegisterSpecialHandler(m_filePath, std::make_unique<CallbackVirtualFileMappingHandler>(
        [this](void* response_object)
        {
            return this->ServeDocumentContent(response_object);
        }));
}


std::wstring DocumentVirtualFileMappingHandler::GetFilePathForDocument(const CDocument& document)
{
    std::wstring file_path = CS2WS(document.GetPathName());

    // if a filename does not exist, create a dummy filename
    if( file_path.empty() )
        return CS2WS(PortableFunctions::FileTempName(GetTempDirectory()));

    return file_path;
}
