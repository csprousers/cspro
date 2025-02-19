#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ApplicationLoader.h>
#include <zAppO/Application.h>


class ZENGINEO_API FileApplicationLoader : public ApplicationLoader
{
public:
    FileApplicationLoader(Application* application, std::optional<std::string> application_file_path = std::nullopt);

    Application* GetApplication() override;

    std::shared_ptr<CDataDict> GetDictionary(const std::string& dictionary_file_path) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const std::string& form_file_path) override;

    std::shared_ptr<CTabSet> GetTableSpec(const std::string& table_spec_file_path) override;

    std::shared_ptr<MessageManager> GetSystemMessages() override;
    std::shared_ptr<MessageManager> GetUserMessages() override;

protected:
    Application* m_application;
    std::optional<std::string> m_applicationFilePathToBeLoaded;
};
