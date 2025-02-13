#include "StdAfx.h"
#include "PdfCreator.h"
#include "GenerateTaskProcessRunner.h"


namespace
{
    constexpr const TCHAR* WkhtmltopdfDisplayText = _T("wkhtmltopdf");
}


PdfCreator::PdfCreator(GenerateTask& generate_task)
    :   m_generateTask(generate_task)
{
    CheckWkhtmltopdfPath(true);
}


const std::wstring& PdfCreator::CreateTemporaryHtmlFilename(const std::wstring& directory, size_t num_docs_to_be_saved_to_file)
{
    const TemporaryFile& temporary_file = m_temporaryHtmlFilenames.emplace_back(TemporaryFile::FromPath(
            PortableFunctions::GetUniqueFilenameInDirectory(directory, FileExtensions::HTML)));

    m_generateTask.GetInterface().LogText(FormatTextCS2WS(_T("\nSaving CSPro Document%s to temporary file: %s"),
                                                          PluralizeWord(num_docs_to_be_saved_to_file),
                                                          temporary_file.GetPath().c_str()));

    return temporary_file.GetPath();
}


const std::wstring& PdfCreator::CreateTemporaryHtmlFilename(size_t num_docs_to_be_saved_to_file)
{
    return CreateTemporaryHtmlFilename(GetTempDirectory(), num_docs_to_be_saved_to_file);
}


void PdfCreator::CheckWkhtmltopdfPath(bool generate_task_interface_may_not_exist) const
{
    if( generate_task_interface_may_not_exist && !m_generateTask.IsInterfaceSet() )
        return;

    if( !PortableFunctions::FileIsRegular(m_generateTask.GetInterface().GetGlobalSettings().wkhtmltopdf_path) )
        throw CSProException(_T("The program %s must be installed to create PDFs. Install the software and then add a reference to it in the Global Settings."), WkhtmltopdfDisplayText);
}


void PdfCreator::CreatePdf(const DocBuildSettings& build_settings, const std::wstring& output_pdf_filename,
                           const std::wstring& contents_html_filename, const std::wstring& cover_page_html_filename/* = std::wstring()*/)
{
    CheckWkhtmltopdfPath(false);

    ASSERT(PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(output_pdf_filename)));
    PortableFunctions::FileDelete(output_pdf_filename);

    std::wstring command_line = EscapeCommandLineArgument(m_generateTask.GetInterface().GetGlobalSettings().wkhtmltopdf_path) +
                                _T(" --enable-local-file-access")
                                _T(" --keep-relative-links");

    auto add_to_command_line = [&](const auto& flag)
    {
        command_line.push_back(' ');
        command_line.append(flag);
    };

    const std::vector<std::tuple<DocBuildSettings::WkhtmltopdfFlagType, std::wstring, std::wstring>>& wkhtmltopdf_flags = build_settings.GetWkhtmltopdfFlags();
    constexpr const TCHAR* TableOfContentsFlag = _T("toc");
    bool add_toc = false;

    auto add_flags = [&](DocBuildSettings::WkhtmltopdfFlagType flags_of_type)
    {
        for( const auto& [type, flag, value] : wkhtmltopdf_flags )
        {
            if( flag == TableOfContentsFlag )
            {
                add_toc = true;
                continue;
            }

            if( type != flags_of_type )
                continue;

            if( type == DocBuildSettings::WkhtmltopdfFlagType::TableOfContents )
                add_toc = true;

            add_to_command_line(flag);

            if( !value.empty() )
                add_to_command_line(value);
        }
    };

    add_flags(DocBuildSettings::WkhtmltopdfFlagType::Global);

    // add the cover page
    if( !cover_page_html_filename.empty() )
    {
        add_to_command_line(_T("cover"));
        add_to_command_line(EscapeCommandLineArgument(cover_page_html_filename));
        add_flags(DocBuildSettings::WkhtmltopdfFlagType::Cover);
    }

    // add the table of contents flag if the user specified one, or specified table of contents flags
    if( add_toc )
    {
        add_to_command_line(L"toc");
        add_flags(DocBuildSettings::WkhtmltopdfFlagType::TableOfContents);
    }

    // add the documents
    add_to_command_line(EscapeCommandLineArgument(contents_html_filename));
    add_flags(DocBuildSettings::WkhtmltopdfFlagType::Page);

    // add the output filename
    add_to_command_line(EscapeCommandLineArgument(output_pdf_filename));

    // create the PDF
    m_generateTask.GetInterface().LogText(_T("\nConverting HTML to PDF using wkhtmltopdf: ") + command_line);

    GenerateTaskProcessRunner process_runner(m_generateTask, WkhtmltopdfDisplayText, WkhtmltopdfDisplayText, &ProcessRunner::ReadStdErr);
    process_runner.Run(std::move(command_line));

    if( m_generateTask.IsCanceled() )
    {
        PortableFunctions::FileDelete(output_pdf_filename);
        return;
    }

    if( !PortableFunctions::FileIsRegular(output_pdf_filename) )
        throw CSProException("There was a problem creating the PDF.");
}
