#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zAppO/Application.h>

class DesignerCompilerMessageProcessor;


class CLASS_DECL_ZSRCMGR DesignerApplicationLoader : public FileApplicationLoader
{
public:
    DesignerApplicationLoader(Application* application, DesignerCompilerMessageProcessor* designer_compiler_message_processor);

    static void ResetCachedObjects();

    Application* GetApplication() override;

    std::shared_ptr<CDataDict> GetDictionary(const std::string& dictionary_file_path) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const std::string& form_file_path) override;

    std::shared_ptr<CTabSet> GetTableSpec(const std::string& table_spec_file_path) override;

    std::shared_ptr<SystemMessageIssuer> GetSystemMessageIssuer() override;

    std::shared_ptr<MessageManager> GetSystemMessages() override;
    std::shared_ptr<MessageManager> GetUserMessages() override;

private:
    DesignerCompilerMessageProcessor* m_designerCompilerMessageProcessor;
};
