#pragma once

#include <zUtilO/TemporaryFile.h>


class PdfCreator
{
public:
    PdfCreator(GenerateTask& generate_task);

    const std::wstring& CreateTemporaryHtmlFilename(const std::wstring& directory, size_t num_docs_to_be_saved_to_file);
    const std::wstring& CreateTemporaryHtmlFilename(size_t num_docs_to_be_saved_to_file);

    void CreatePdf(const DocBuildSettings& build_settings, const std::wstring& output_pdf_filename,
                   const std::wstring& contents_html_filename, const std::wstring& cover_page_html_filename = std::wstring());

private:
    void CheckWkhtmltopdfPath(bool generate_task_interface_may_not_exist) const;

private:
    GenerateTask& m_generateTask;
    std::vector<TemporaryFile> m_temporaryHtmlFilenames;
};
