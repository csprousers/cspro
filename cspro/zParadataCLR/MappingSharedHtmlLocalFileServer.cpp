#include "Stdafx.h"
#include "MappingSharedHtmlLocalFileServer.h"
#include <zHtml/SharedHtmlLocalFileServer.h>


CSPro::ParadataViewer::MappingSharedHtmlLocalFileServer::MappingSharedHtmlLocalFileServer()
    :   m_fileServer(new SharedHtmlLocalFileServer("mapping"))
{
}


CSPro::ParadataViewer::MappingSharedHtmlLocalFileServer::!MappingSharedHtmlLocalFileServer()
{
    delete m_fileServer;
}


System::String^ CSPro::ParadataViewer::MappingSharedHtmlLocalFileServer::CreateProjectUrl(System::String^ url_from_project_root)
{
    return clr_helpers::to_SystemString(m_fileServer->CreateProjectUrl(clr_helpers::to_string(url_from_project_root)));
}
