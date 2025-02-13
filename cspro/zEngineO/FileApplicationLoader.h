#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ApplicationLoader.h>
#include <zAppO/Application.h>


class ZENGINEO_API FileApplicationLoader : public ApplicationLoader
{
public:
    FileApplicationLoader(Application* application, std::optional<CString> application_filename = std::nullopt);

    Application* GetApplication() override;

    std::shared_ptr<CDataDict> GetDictionary(NullTerminatedString dictionary_filename) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const CString& form_filename) override;

    std::shared_ptr<MessageManager> GetSystemMessages() override;
    std::shared_ptr<MessageManager> GetUserMessages() override;

protected:
    Application* m_application;
    std::optional<CString> m_applicationFilenameToBeLoaded;
};
