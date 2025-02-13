#include "StdAfx.h"
#include "CommandLineBuilder.h"
#include "CommandLineParser.h"
#include "ConsoleWrapper.h"
#include "CSDocCompiler.h"
#include "DocSetBuilder.h"
#include "DocSetBuilderCache.h"


CommandLineBuilder::CommandLineBuilder(GlobalSettings& global_settings)
    :   m_globalSettings(global_settings),
        m_console(nullptr)
{
}


void CommandLineBuilder::Build(const CommandLineParser& command_line_parser)
{
    ASSERT(!command_line_parser.GetInputFilenames().empty());

    ConsoleWrapper console;
    m_console = &console;

    // reset the cache before the builds
    CacheableCalculator::ResetCache();

    try
    {
        const int64_t start_timestamp = GetTimestamp<int64_t>();
        const std::vector<std::wstring>& input_filenames = command_line_parser.GetInputFilenames();

        for( const std::wstring& input_filename : input_filenames )
        {
            std::unique_ptr<GenerateTask> generate_task = CreateGenerateTask(command_line_parser, input_filename);
            ASSERT(generate_task != nullptr);

            // run the task
            m_completionStatus.reset();

            generate_task->SetInterface(*this);
            generate_task->Run();

            while( !m_completionStatus.has_value() )
            {
                if( console.IsUserCancelingProgramAndReadInput() )
                    generate_task->Cancel();

                constexpr DWORD SleepMilliseconds = 50;
                Sleep(SleepMilliseconds);
            }

            if( *m_completionStatus == GenerateTask::Status::Canceled )
                throw CSProException("Build canceled!");

            if( *m_completionStatus == GenerateTask::Status::EndedInException )
                throw CSProException(_T("Build canceled due to errors building: ") + input_filename);
        }

        m_console->WriteLine(FormatTextCS2WS(_T("\nCommand line build of %d input%s completed in %s."),
                                             static_cast<int>(input_filenames.size()), PluralizeWord(input_filenames.size()),
                                             GenerateTask::GetElapsedTimeText(start_timestamp).c_str()));

        if( !m_finalOutputs.empty() )
        {
            m_console->WriteLine(_T("\nThe following outputs were successfully created:\n"));

            for( const auto& [output_title, path] : m_finalOutputs )
            {
                std::wstring output_description = _T("  * ") + output_title;

                if( output_title != path )
                    SO::Append(output_description, _T(" -> "), path);

                m_console->WriteLine(output_description);
            }
        }
    }

    catch( const CSProException& exception )
    {
        m_console->WriteLine();
        console.WriteLine(exception.GetErrorMessage());
        throw;
    }
}


std::unique_ptr<GenerateTask> CommandLineBuilder::CreateGenerateTask(const CommandLineParser& command_line_parser, const std::wstring& input_filename)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(input_filename);
    const bool is_csdoc = SO::EqualsNoCase(extension, FileExtensions::CSDocument);

    if( !is_csdoc && !SO::EqualsNoCase(extension, FileExtensions::CSDocumentSet) )
        throw CSProException(_T("There is no routine for building files with the extension: ") + extension);

    // read and compile the Document Set for settings (if applicable)
    auto get_doc_set = [](const std::wstring& doc_set_filename)
    {
        auto doc_set_spec = std::make_shared<DocSetSpec>(doc_set_filename);

        DocSetCompiler doc_set_compiler(DocSetCompiler::ThrowErrors { });
        doc_set_compiler.CompileSpec(*doc_set_spec, FileIO::ReadText(doc_set_filename), DocSetCompiler::SpecCompilationType::SettingsOnly);

        return doc_set_spec;
    };

    std::shared_ptr<DocSetSpec> doc_set_spec = !is_csdoc                                        ? get_doc_set(input_filename) :
                                               !command_line_parser.GetDocSetFilename().empty() ? get_doc_set(command_line_parser.GetDocSetFilename()) :
                                                                                                  nullptr;

    // read any custom build settings
    std::unique_ptr<DocSetSettings> custom_doc_set_settings;

    if( !command_line_parser.GetBuildSettingsFilename().empty() )
    {
        custom_doc_set_settings = std::make_unique<DocSetSettings>(DocSetCompiler::GetSettingsFromSpecOrSettingsFile(command_line_parser.GetBuildSettingsFilename()));

        if( !custom_doc_set_settings->GetDefaultBuildSettings() && custom_doc_set_settings->GetNamedBuildSettings().empty() )
            throw CSProException(_T("There are no build settings in the file: ") + command_line_parser.GetBuildSettingsFilename());
    }

    // get the build settings
    const DocSetSettings* doc_set_settings_for_build = ( custom_doc_set_settings != nullptr ) ? custom_doc_set_settings.get() :
                                                       ( doc_set_spec != nullptr )            ? &doc_set_spec->GetSettings() :
                                                                                                nullptr;

    const bool require_that_doc_set_settings_has_build_settings = ( custom_doc_set_settings != nullptr );

    auto [build_settings, build_name] = GetBuildSettings(doc_set_settings_for_build, command_line_parser.GetBuildNameOrType(),
                                                         require_that_doc_set_settings_has_build_settings);

    return is_csdoc ? CreateGenerateTaskForCSDoc(std::move(doc_set_spec), std::move(build_settings), input_filename, command_line_parser.GetOutputPath()) :
                      CreateGenerateTaskForDocumentSet(std::move(doc_set_spec), std::move(build_settings), std::move(build_name), command_line_parser.GetOutputPath());
}


std::tuple<DocBuildSettings, std::wstring> CommandLineBuilder::GetBuildSettings(const DocSetSettings* doc_set_settings, const std::wstring& build_name_or_type,
                                                                                bool require_that_doc_set_settings_has_build_settings)
{
    // when a build name or type is specified, it must exist as a name or as one of the build types
    if( !build_name_or_type.empty() )
    {
        const std::optional<DocBuildSettings::BuildType> build_type = FromString<DocBuildSettings::BuildType>(build_name_or_type);

        if( build_type.has_value() )
        {
            return ( doc_set_settings == nullptr ) ? std::make_tuple(DocBuildSettings::DefaultSettingsForBuildType(*build_type), std::wstring()) :
                                                     doc_set_settings->GetEvaluatedBuildSettings(*build_type, build_name_or_type);
        }

        else if( doc_set_settings != nullptr )
        {
            return std::make_tuple(doc_set_settings->GetEvaluatedBuildSettings(build_name_or_type), build_name_or_type);
        }

        else
        {
            throw CSProException("You cannot specify the name of build settings without also specifying a Document Set or settings file.");            
        }
    }

    // if no build name or type is used, use the default settings if set
    else if( doc_set_settings != nullptr )
    {
        if( doc_set_settings->GetDefaultBuildSettings().has_value() )
            return std::make_tuple(*doc_set_settings->GetDefaultBuildSettings(), std::wstring());

        if( require_that_doc_set_settings_has_build_settings )
            throw CSProException("There are no default build settings in the Document Set or settings file.");        
    }

    // otherwise return default settings
    return std::tuple<DocBuildSettings, std::wstring>();
}


std::unique_ptr<GenerateTask> CommandLineBuilder::CreateGenerateTaskForCSDoc(std::shared_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                             const std::wstring& csdoc_filename, const std::wstring& output_path)
{
    if( doc_set_spec == nullptr )
        doc_set_spec = std::make_shared<DocSetSpec>();

    std::wstring output_filename = output_path;

    // if no output filename is given, create one in the same directory as the input;
    // if the output path is a directory, create a filename in that directory
    if( output_filename.empty() || PortableFunctions::FileIsDirectory(output_path) )
    {
        if( output_filename.empty() )
            output_filename = PortableFunctions::PathGetDirectory(csdoc_filename);

        const TCHAR* expected_extension = ( build_settings.GetBuildType() == DocBuildSettings::BuildType::Pdf ) ? FileExtensions::WithDot::PDF :
                                                                                                                  FileExtensions::WithDot::HTML;

        output_filename = PortableFunctions::PathAppendToPath(output_filename,
                                                              PortableFunctions::PathGetFilenameWithoutExtension(csdoc_filename) + expected_extension);
    }

    return std::make_unique<CSDocCompilerBuildToFileGenerateTask>(std::move(doc_set_spec), build_settings,
                                                                  csdoc_filename, FileIO::ReadText(csdoc_filename),
                                                                  std::move(output_filename));
}


std::unique_ptr<GenerateTask> CommandLineBuilder::CreateGenerateTaskForDocumentSet(std::shared_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                                   std::wstring build_name, const std::wstring& output_path)
{
    ASSERT(doc_set_spec != nullptr);

    if( !output_path.empty() )
        build_settings.SetOutputDirectory(output_path);

    std::unique_ptr<DocSetBuilderBaseGenerateTask> generate_task =
        DocSetBuilderBaseGenerateTask::CreateForBuild(std::move(doc_set_spec), build_settings, std::move(build_name), true);

    // the build cache will be shared among all Document Set builds
    if( m_docSetBuilderCache == nullptr )
        m_docSetBuilderCache = std::make_shared<DocSetBuilderCache>();

    generate_task->SetDocSetBuilderCache(m_docSetBuilderCache);

    return generate_task;
}


void CommandLineBuilder::SetTitle(const std::wstring& title)
{
    m_console->WriteLine();
    m_console->WriteLine(title);
    m_console->WriteLine(SO::GetDashedLine(title.length()));
    m_console->WriteLine();
}


void CommandLineBuilder::LogText(std::wstring text)
{
    m_console->WriteLine(text);
}


void CommandLineBuilder::UpdateProgress(double /*percent*/)
{
}


void CommandLineBuilder::SetOutputText(const std::wstring& /*text*/)
{
}


void CommandLineBuilder::OnCreatedOutput(std::wstring output_title, std::wstring path)
{
    m_finalOutputs.emplace_back(std::move(output_title), std::move(path));
}


void CommandLineBuilder::OnException(const CSProException& exception)
{
    const std::wstring error = exception.GetErrorMessage();
    const std::wstring line = SO::GetDashedLine(std::min<size_t>(80, std::max<size_t>(30, error.length())));

    m_console->WriteLine();
    m_console->WriteLine(line);
    LogText(error);
    m_console->WriteLine(line);
}
    

void CommandLineBuilder::OnCompletion(GenerateTask::Status status)
{
    m_completionStatus = status;
}


const GlobalSettings& CommandLineBuilder::GetGlobalSettings()
{
    return m_globalSettings;
}
