#pragma once

#include <zFormO/FormFile.h>


class ExampleFileUpdater
{
public:
    void Update(const std::string& examples_directory);

    void MarkAsUpdated(const std::string& file_path);

private:
    void UpdateApplication(const std::string& application_file_path);

    std::shared_ptr<CDEFormFile> UpdateFormFile(const std::string& form_file_path);
    
    void UpdateQuestionText(const std::string& qsf_file_path, const std::vector<std::shared_ptr<CDEFormFile>>& form_files);

    void UpdateTabSpec(const std::string& tab_spec_file_path);

    void UpdateObjectlessFile(const std::string& file_path);

private:
    std::map<std::string, size_t> m_updateCounts;
    std::map<std::string, std::shared_ptr<CDEFormFile>> m_formFilesProcessed;
};
