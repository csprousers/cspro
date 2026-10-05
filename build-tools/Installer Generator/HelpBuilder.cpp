#include "Stdafx.h"
#include "HelpBuilder.h"
#include <zUtilO/FileExtensions.h>
#include <zDocument/BuildToolDocSetBuilder.h>


class CSPro::InstallerGenerator::HelpBuilderWorker
{
public:
    HelpBuilderWorker(std::string helps_directory, gcroot<OnLogText^> on_log_text_callback);

    void Create(const std::string& program);

private:
    std::string m_helpsDirectory;
    gcroot<OnLogText^> m_onLogTextCallback;
    BuildToolDocSetBuilder m_buildToolDocSetBuilder;
};


CSPro::InstallerGenerator::HelpBuilderWorker::HelpBuilderWorker(std::string helps_directory, gcroot<OnLogText^> on_log_text_callback)
    :   m_helpsDirectory(std::move(helps_directory)),
        m_onLogTextCallback(std::move(on_log_text_callback))
{
    ASSERT(PortableFunctions::FileIsDirectory(m_helpsDirectory));

    if( static_cast<OnLogText^>(m_onLogTextCallback) != nullptr )
    {
        m_buildToolDocSetBuilder.SetLogger(
            [this](const SharableString text) { m_onLogTextCallback->Invoke(clr_helpers::to_SystemString(*text)); }
        );
    }
}


void CSPro::InstallerGenerator::HelpBuilderWorker::Create(const std::string& program)
{
    std::string csdocset_file_path = Path::Combine(
        m_helpsDirectory,
        program,
        Path::AppendExtension(program, FileExtensions::CSDocumentSet)
    );

    m_buildToolDocSetBuilder.BuildDocSet(
        std::move(csdocset_file_path),
        std::string(), // with the output directory empty, it will use the directory specified in helps/Shared/settings.json
        "Compiled HTML Help"
    );
}


CSPro::InstallerGenerator::HelpBuilder::HelpBuilder(System::String^ helps_directory, OnLogText^ on_log_text_callback)
{
    try
    {
        m_worker = new HelpBuilderWorker(clr_helpers::to_string(helps_directory), on_log_text_callback);
    }

    catch( const std::exception& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}


CSPro::InstallerGenerator::HelpBuilder::!HelpBuilder()
{
    delete m_worker;
}


void CSPro::InstallerGenerator::HelpBuilder::Create(System::String^ program)
{
    try
    {
        m_worker->Create(clr_helpers::to_string(program));
    }

    catch( const std::exception& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}
