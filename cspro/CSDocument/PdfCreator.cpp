#include "StdAfx.h"
#include "PdfCreator.h"
#include "GenerateTaskProcessRunner.h"


namespace
{
    constexpr const char* WkhtmltopdfDisplayText = "wkhtmltopdf";
}


PdfCreator::PdfCreator(GenerateTask& generate_task)
    :   m_generateTask(generate_task)
{
    CheckWkhtmltopdfPath(true);
}


const std::string& PdfCreator::CreateTemporaryHtmlFilePath(const std::string& directory_path, const size_t num_docs_to_be_saved_to_file)
{
    std::string temp_file_path = PortableFunctions::GetUniqueFilePathInDirectory(directory_path, FileExtensions::HTML);
    const TemporaryFile& temporary_file = m_temporaryHtmlFilePaths.emplace_back(TemporaryFile::FromPath(std::move(temp_file_path)));

    m_generateTask.GetInterface().LogText("\nSaving CSPro Document%s to temporary file: %s",
                                          PluralizeWord(num_docs_to_be_saved_to_file),
                                          temporary_file.GetPath().c_str());

    return temporary_file.GetPath();
}


const std::string& PdfCreator::CreateTemporaryHtmlFilePath(const size_t num_docs_to_be_saved_to_file)
{
    return CreateTemporaryHtmlFilePath(GetTempDirectory(), num_docs_to_be_saved_to_file);
}


void PdfCreator::CheckWkhtmltopdfPath(const bool generate_task_interface_may_not_exist) const
{
    if( generate_task_interface_may_not_exist && !m_generateTask.IsInterfaceSet() )
        return;

    if( !PortableFunctions::FileIsRegular(m_generateTask.GetInterface().GetGlobalSettings().wkhtmltopdf_path) )
    {
        throw CSProException("The program %s must be installed to create PDFs. "
                             "Install the software and then add a reference to it in the Global Settings.",
                             WkhtmltopdfDisplayText);
    }
}


void PdfCreator::CreatePdf(const DocBuildSettings& build_settings, const std::string& output_pdf_file_path,
                           const std::string& contents_html_file_path, const std::string& cover_page_html_file_path/* = std::string()*/)
{
    CheckWkhtmltopdfPath(false);

    ASSERT(PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(output_pdf_file_path)));
    PortableFunctions::FileDelete(output_pdf_file_path);

    std::string command_line = EscapeCommandLineArgument(m_generateTask.GetInterface().GetGlobalSettings().wkhtmltopdf_path) +
                               " --enable-local-file-access"
                               " --keep-relative-links";

    auto add_to_command_line = [&](const auto& flag)
    {
        command_line.push_back(' ');
        command_line.append(flag);
    };

    const std::vector<std::tuple<DocBuildSettings::WkhtmltopdfFlagType, std::string, std::string>>& wkhtmltopdf_flags = build_settings.GetWkhtmltopdfFlags();
    constexpr const char* TableOfContentsFlag = "toc";
    bool add_toc = false;

    auto add_flags = [&](const DocBuildSettings::WkhtmltopdfFlagType flags_of_type)
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
    if( !cover_page_html_file_path.empty() )
    {
        add_to_command_line("cover");
        add_to_command_line(EscapeCommandLineArgument(cover_page_html_file_path));
        add_flags(DocBuildSettings::WkhtmltopdfFlagType::Cover);
    }

    // add the table of contents flag if the user specified one, or specified table of contents flags
    if( add_toc )
    {
        add_to_command_line("toc");
        add_flags(DocBuildSettings::WkhtmltopdfFlagType::TableOfContents);
    }

    // add the documents
    add_to_command_line(EscapeCommandLineArgument(contents_html_file_path));
    add_flags(DocBuildSettings::WkhtmltopdfFlagType::Page);

    // add the output filename
    add_to_command_line(EscapeCommandLineArgument(output_pdf_file_path));

    // create the PDF
    m_generateTask.GetInterface().LogText("\nConverting HTML to PDF using wkhtmltopdf: " + command_line);

    GenerateTaskProcessRunner process_runner(m_generateTask, WkhtmltopdfDisplayText, WkhtmltopdfDisplayText, &ProcessRunner::ReadStdErr);
    process_runner.Run(command_line);

    if( m_generateTask.IsCanceled() )
    {
        PortableFunctions::FileDelete(output_pdf_file_path);
        return;
    }

    if( !PortableFunctions::FileIsRegular(output_pdf_file_path) )
        throw CSProException("There was a problem creating the PDF.");
}
