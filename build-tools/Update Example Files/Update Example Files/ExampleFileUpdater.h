#pragma once

#include <zFormO/FormFile.h>


class ExampleFileUpdater
{
public:
    void Update(const std::wstring& examples_directory);

    void MarkAsUpdated(const std::wstring& filename);

private:
    void UpdateApplication(const std::wstring& application_filename);

    std::shared_ptr<CDEFormFile> UpdateFormFile(const std::wstring& form_filename);
    
    void UpdateQuestionText(const std::wstring& qsf_filename, const std::vector<std::shared_ptr<CDEFormFile>>& form_files);

    void UpdateTabSpec(const std::wstring& tab_spec_filename);

    void UpdateObjectlessFile(const std::wstring& filename);

private:
    std::map<std::wstring, size_t> m_updateCounts;
    std::map<std::wstring, std::shared_ptr<CDEFormFile>> m_formFilesProcessed;
};
