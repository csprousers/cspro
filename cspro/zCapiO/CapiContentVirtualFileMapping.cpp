#include "StdAfx.h"
#include "CapiContentVirtualFileMapping.h"
#include "CapiContent.h"
#include "CapiQuestionManager.h"
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Application.h>


const std::string* CapiContentVirtualFileMapping::GetQuestionTextUrl() const
{
    return ( m_questionTextVirtualFileMapping != nullptr ) ? &m_questionTextVirtualFileMapping->GetUrl() :
                                                             nullptr;
}


const std::string* CapiContentVirtualFileMapping::GetHelpTextUrl() const
{
    return ( m_helpTextVirtualFileMapping != nullptr ) ? &m_helpTextVirtualFileMapping->GetUrl() :
                                                         nullptr;
}


void CapiContentVirtualFileMapping::SetCapiContent(CapiContent capi_content, const Application& application,
                                                   std::shared_ptr<CapiQuestionManager> question_manager)
{
    ASSERT(question_manager != nullptr);

    if( capi_content.question_text->empty() && capi_content.help_text->empty() )
        return;

    const std::string directory = PortableFunctions::PathGetDirectory(application.GetQuestionTextFilePath());

    if( !capi_content.question_text->empty() )
        m_questionTextVirtualFileMapping = CreateVirtualFileMapping(question_manager, directory, std::move(capi_content.question_text));

    if( !capi_content.help_text->empty() )
        m_helpTextVirtualFileMapping = CreateVirtualFileMapping(std::move(question_manager), directory, std::move(capi_content.help_text));
}


std::unique_ptr<VirtualFileMapping> CapiContentVirtualFileMapping::CreateVirtualFileMapping(std::shared_ptr<CapiQuestionManager> question_manager,
                                                                                            const std::string& directory, SharableString content)
{
    ASSERT(question_manager != nullptr);

    return std::make_unique<VirtualFileMapping>(PortableLocalhost::CreateVirtualHtmlFile(directory,
        [question_manager_ = std::move(question_manager), html = SharableString(), content_ = std::move(content)]() mutable
        {
            if( !html.IsSet() )
            {
                html = SO::Concatenate("<!doctype html><html><head><meta charset=\"utf-8\">"
                                       "<style>"
                                           "body{background-color:#EFEFEF;margin:0;padding:0}"
                                           "table{width: 100%;border-collapse: collapse;}"
                                           "td,th{border: 1px solid #7B7B7B;padding: 5px 3px;}"
                                       "</style>"
                                       "<style>",
                                           question_manager_->GetRuntimeStylesCss(),
                                       "</style>"
                                       "</head><body>",
                                           *content_,
                                       "</body></html>");
            }

            return html;
        }));
}
