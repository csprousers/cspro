#pragma once

#include <Zentryo/zEntryO.h>

struct CapiContent;
class CEntryDriver;
class VirtualFileMapping;


class CLASS_DECL_ZENTRYO CapiContentVirtualFileMapping
{
public:
    // a URL is only defined when there is content
    std::optional<std::wstring> GetQuestionTextUrl() const;
    std::optional<std::wstring> GetHelpTextUrl() const;

    void SetCapiContent(CapiContent capi_content, CEntryDriver& entry_driver);

private:
    std::shared_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;
    std::shared_ptr<VirtualFileMapping> m_helpTextVirtualFileMapping;
};
