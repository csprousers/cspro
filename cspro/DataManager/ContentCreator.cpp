#include "StdAfx.h"
#include "ContentCreator.h"
#include <zHtml/VirtualFileMapping.h>


ContentCreator::~ContentCreator()
{
}


std::vector<const char*> ContentCreator::GetSaveFormats() const
{
    return { FileExtensions::HTML };
}


bool ContentCreator::UsesActionInvoker() const
{
    return false;
}


SharableString ContentCreator::GetActionInvokerInputData()
{
    throw ProgrammingErrorException();
}


UINT ContentCreator::GetViewOptionsMenuResourceId() const
{
    return 0;
}


bool ContentCreator::ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> /*data*/)
{
    return false;
}


void ContentCreator::ProcessWebViewMessage(const JsonNode& json_node)
{
    if( json_node.Contains(JK::action) )
    {
        throw CSProException("The view does not support a web message with the action '%s'.",
                             json_node.Get<std::string>(JK::action).c_str());
    }

    throw CSProException("The view does not support web messages.");
}



SharableString ContentCreator::GetTextContent()
{
    throw ProgrammingErrorException();
}


SharableString ContentCreator::GetHtmlContent(bool /*embed_resources = false*/)
{
    throw ProgrammingErrorException();
}


const std::string& ContentCreator::GetUrl()
{
    GetUrlWorker();
    ASSERT(m_htmlContentServer.has_value());

    if( std::holds_alternative<std::unique_ptr<VirtualFileMappingHandler>>(*m_htmlContentServer) )
    {
        ASSERT(std::get<std::unique_ptr<VirtualFileMappingHandler>>(*m_htmlContentServer) != nullptr);
        return std::get<std::unique_ptr<VirtualFileMappingHandler>>(*m_htmlContentServer)->GetUrl();
    }

    else
    {
        return std::get<VirtualFileMapping>(*m_htmlContentServer).GetUrl();
    }
}


void ContentCreator::GetUrlWorker()
{
    m_htmlContentServer = std::make_unique<TextVirtualFileMappingHandler>(GetHtmlContent(), MimeType::Type::Html);

    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();
    file_server.CreateVirtualFile(*std::get<std::unique_ptr<VirtualFileMappingHandler>>(*m_htmlContentServer));
}
