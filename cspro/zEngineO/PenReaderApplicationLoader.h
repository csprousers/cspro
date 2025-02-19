#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ApplicationLoader.h>
#include <zAppO/Application.h>


class ZENGINEO_API PenReaderApplicationLoader : public ApplicationLoader
{
public:
    PenReaderApplicationLoader(Application* application, std::string pen_file_path);
    ~PenReaderApplicationLoader();

    Application* GetApplication() override;

    std::shared_ptr<CDataDict> GetDictionary(const std::string& dictionary_file_path) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const std::string& form_file_path) override;

    std::shared_ptr<CTabSet> GetTableSpec(const std::string& table_spec_file_path) override;

    std::shared_ptr<MessageManager> GetSystemMessages() override;
    std::shared_ptr<MessageManager> GetUserMessages() override;

    void ProcessUserMessagesPostCompile(MessageManager& user_message_manager) override;

    void ProcessResources() override;

private:
    Application* m_application;
    std::unique_ptr<Serializer> m_serializer;
    Serializer* m_serializer_APP_LOAD_TODO;
};
