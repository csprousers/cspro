#include "Stdafx.h"
#include "MappingSharedHtmlLocalFileServer.h"
#include <zHtml/SharedHtmlLocalFileServer.h>

namespace CSPro
{
    namespace ParadataViewer
    {
        MappingSharedHtmlLocalFileServer::MappingSharedHtmlLocalFileServer()
            :   m_fileServer(new SharedHtmlLocalFileServer(_T("mapping")))
        {
        }

        MappingSharedHtmlLocalFileServer::!MappingSharedHtmlLocalFileServer()
        {
            delete m_fileServer;
        }

        System::String^ MappingSharedHtmlLocalFileServer::GetProjectUrl(System::String^ url_from_project_root)
        {
            return gcnew System::String(m_fileServer->GetProjectUrl(CS2WS(url_from_project_root)).c_str());
        }
    }
}
