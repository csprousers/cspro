#pragma once

#include <zUtilO/TemporaryFile.h>


class PdfCreator
{
public:
    PdfCreator(GenerateTask& generate_task);

    const std::string& CreateTemporaryHtmlFilePath(const std::string& directory_path, size_t num_docs_to_be_saved_to_file);
    const std::string& CreateTemporaryHtmlFilePath(size_t num_docs_to_be_saved_to_file);

    void CreatePdf(const DocBuildSettings& build_settings, const std::string& output_pdf_file_path,
                   const std::string& contents_html_file_path, const std::string& cover_page_html_file_path = std::string());

private:
    void CheckWkhtmltopdfPath(bool generate_task_interface_may_not_exist) const;

private:
    GenerateTask& m_generateTask;
    std::vector<TemporaryFile> m_temporaryHtmlFilePaths;
};
