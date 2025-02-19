#pragma once

#include <Zentryo/zEntryO.h>

struct CapiContent;
class CEntryDriver;
class VirtualFileMapping;


class CLASS_DECL_ZENTRYO CapiContentVirtualFileMapping
{
public:
    // a URL is only non-null when there is content
    const std::string* GetQuestionTextUrl() const;
    const std::string* GetHelpTextUrl() const;

    void SetCapiContent(const CapiContent& capi_content, CEntryDriver& entry_driver);

private:
    std::unique_ptr<VirtualFileMapping> CreateVirtualFileMapping(CEntryDriver& entry_driver, const std::string& directory, SharableString content);

private:
    std::shared_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;
    std::shared_ptr<VirtualFileMapping> m_helpTextVirtualFileMapping;
};
