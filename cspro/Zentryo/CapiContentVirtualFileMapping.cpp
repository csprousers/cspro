#include "StdAfx.h"
#include "CapiContentVirtualFileMapping.h"
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Application.h>
#include <zCapiO/CapiContent.h>
#include <zCapiO/CapiQuestionManager.h>
#include <engine/Entdrv.h>


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


void CapiContentVirtualFileMapping::SetCapiContent(const CapiContent& capi_content, CEntryDriver& entry_driver)
{
    if( capi_content.question_text.IsEmpty() && capi_content.help_text.IsEmpty() )
        return;

    const std::string directory = PortableFunctions::PathGetDirectory(entry_driver.GetApplication()->GetQuestionTextFilePath());

    if( !capi_content.question_text.IsEmpty() )
        m_questionTextVirtualFileMapping = CreateVirtualFileMapping(entry_driver, directory, UTF8_TODO::GetUtf8(capi_content.question_text));

    if( !capi_content.help_text.IsEmpty() )
        m_helpTextVirtualFileMapping = CreateVirtualFileMapping(entry_driver, directory, UTF8_TODO::GetUtf8(capi_content.help_text));
}


std::unique_ptr<VirtualFileMapping> CapiContentVirtualFileMapping::CreateVirtualFileMapping(CEntryDriver& entry_driver, const std::string& directory, SharableString content)
{
    return std::make_unique<VirtualFileMapping>(PortableLocalhost::CreateVirtualHtmlFile(directory,
        [&entry_driver, html = SharableString(), content_ = std::move(content)]() mutable
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
                                           entry_driver.GetQuestMgr()->GetRuntimeStylesCss(),
                                       "</style>"
                                       "</head><body>",
                                           *content_,
                                       "</body></html>");
            }

            return html;
        }));
}


