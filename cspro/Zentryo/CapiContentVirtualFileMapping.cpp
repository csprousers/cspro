#include "StdAfx.h"
#include "CapiContentVirtualFileMapping.h"
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Application.h>
#include <zCapiO/CapiContent.h>
#include <zCapiO/CapiQuestionManager.h>
#include <engine/Entdrv.h>


namespace
{
    std::unique_ptr<VirtualFileMapping> CreateCapiContentVirtualFileMapping(CEntryDriver& entry_driver, const std::wstring& directory, std::wstring content)
    {
        return std::make_unique<VirtualFileMapping>(PortableLocalhost::CreateVirtualHtmlFile(directory,
            [&entry_driver, content = std::move(content)]() ->std::string
            {
                return "<!doctype html><html><head><meta charset=\"utf-8\">"
                       "<style>"
                            "body{background-color:#EFEFEF;margin:0;padding:0}"
                            "table{width: 100%;border-collapse: collapse;}"
                            "td,th{border: 1px solid #7B7B7B;padding: 5px 3px;}"
                        "</style>"
                       "<style>" + UTF8Convert::WideToUTF8(entry_driver.GetQuestMgr()->GetRuntimeStylesCss()) + "</style>"
                       "</head><body>" + UTF8Convert::WideToUTF8(content) + "</body></html>";
            }));
    }
}


std::optional<std::wstring> CapiContentVirtualFileMapping::GetQuestionTextUrl() const
{
    return ( m_questionTextVirtualFileMapping != nullptr ) ? std::make_optional(m_questionTextVirtualFileMapping->GetUrl()) :
                                                             std::nullopt;
}


std::optional<std::wstring> CapiContentVirtualFileMapping::GetHelpTextUrl() const
{
    return ( m_helpTextVirtualFileMapping != nullptr ) ? std::make_optional(m_helpTextVirtualFileMapping->GetUrl()) :
                                                         std::nullopt;
}


void CapiContentVirtualFileMapping::SetCapiContent(CapiContent capi_content, CEntryDriver& entry_driver)
{
    if( capi_content.question_text.IsEmpty() && capi_content.help_text.IsEmpty() )
        return;

    std::wstring directory = PortableFunctions::PathGetDirectory(entry_driver.GetApplication()->GetQuestionTextFilename());

    if( !capi_content.question_text.IsEmpty() )
        m_questionTextVirtualFileMapping = CreateCapiContentVirtualFileMapping(entry_driver, directory, CS2WS(capi_content.question_text));

    if( !capi_content.help_text.IsEmpty() )
        m_helpTextVirtualFileMapping = CreateCapiContentVirtualFileMapping(entry_driver, directory, CS2WS(capi_content.help_text));
}
