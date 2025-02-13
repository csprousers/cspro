#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ApplicationLoader.h>
#include <zAppO/Application.h>

class Serializer;


class ZENGINEO_API PenReaderApplicationLoader : public ApplicationLoader
{
public:
    PenReaderApplicationLoader(Application* application, std::wstring pen_filename);
    ~PenReaderApplicationLoader();

    Application* GetApplication() override;

    std::shared_ptr<CDataDict> GetDictionary(NullTerminatedString dictionary_filename) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const CString& form_filename) override;

    std::shared_ptr<MessageManager> GetSystemMessages() override;
    std::shared_ptr<MessageManager> GetUserMessages() override;

    void ProcessUserMessagesPostCompile(MessageManager& user_message_manager) override;

    void ProcessResources() override;

private:
    Application* m_application;
    std::unique_ptr<Serializer> m_serializer;
    Serializer* m_serializer_APP_LOAD_TODO;
};
