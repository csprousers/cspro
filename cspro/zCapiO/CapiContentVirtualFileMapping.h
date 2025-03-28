#pragma once

#include <zCapiO/zCapiO.h>

class Application;
struct CapiContent;
class CapiQuestionManager;
class VirtualFileMapping;


class CLASS_DECL_ZCAPIO CapiContentVirtualFileMapping
{
public:
    // a URL is only non-null when there is content
    const std::string* GetQuestionTextUrl() const;
    const std::string* GetHelpTextUrl() const;

    void SetCapiContent(CapiContent capi_content, const Application& application, std::shared_ptr<CapiQuestionManager> question_manager);

private:
    std::unique_ptr<VirtualFileMapping> CreateVirtualFileMapping(std::shared_ptr<CapiQuestionManager> question_manager,
                                                                 const std::string& directory, SharableString content);

private:
    std::shared_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;
    std::shared_ptr<VirtualFileMapping> m_helpTextVirtualFileMapping;
};
