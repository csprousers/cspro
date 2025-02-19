#include "stdafx.h"
#include "DocumentVirtualFileMappingHandler.h"
#include <zToolsO/FileIO.h>


DocumentVirtualFileMappingHandler::DocumentVirtualFileMappingHandler(const CDocument& document)
    :   m_document(document),
        m_filePath(GetFilePathForDocument(m_document))
{
    RegisterSpecialHandler(m_filePath, std::make_unique<CallbackVirtualFileMappingHandler>(
        [this](VirtualFileMappingResponse& response)
        {
            return this->ServeDocumentContent(response);
        }));
}


std::string DocumentVirtualFileMappingHandler::GetFilePathForDocument(const CDocument& document)
{
    std::string file_path = TC::ToUtf8(document.GetPathName());

    // if a file path does not exist, create a dummy file path
    return file_path.empty() ? PortableFunctions::FileTempPath(GetTempDirectory()) :
                               file_path;
}
