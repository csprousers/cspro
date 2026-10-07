#include "StdAfx.h"
#include "CSDocCompilerSettings.h"
#include "DocSetBuilderCache.h"


CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                   std::string build_name)
    :   CSDocCompilerSettings(std::move(doc_set_spec)),
        m_buildSettings(std::move(build_settings)),
        m_buildName(std::move(build_name)),
        m_docSetBuilderCache(std::make_shared<DocSetBuilderCache>())
{
}


std::unique_ptr<CSDocCompilerSettingsForBuilding>
CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, const DocBuildSettings& base_build_settings,
                                                       const DocBuildSettings::BuildType build_type, std::string build_name,
                                                       const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
{
    // create build settings for the build type and apply any user build settings on top
    DocBuildSettings build_settings = DocBuildSettings::ApplySettings(DocBuildSettings::DefaultSettingsForBuildType(build_type), base_build_settings);

    // make sure the build type is properly set (in case it was overridden by ApplySettings)
    build_settings.SetBuildType(build_type);

    // fix any issues with the settings
    build_settings.FixIncompatibleSettingsForBuildType(throw_exceptions_for_serious_issues_when_validating_build_settings);

    // create the settings
    std::unique_ptr<CSDocCompilerSettingsForBuilding> settings;

    switch( build_type )
    {
        case DocBuildSettings::BuildType::HtmlPages:
            settings = std::make_unique<CSDocCompilerSettingsForBuildingHtmlPages>(std::move(doc_set_spec), std::move(build_settings), std::move(build_name));
            break;

        case DocBuildSettings::BuildType::HtmlWebsite:
            settings = std::make_unique<CSDocCompilerSettingsForBuildingHtmlWebsite>(std::move(doc_set_spec), std::move(build_settings), std::move(build_name));
            break;

        case DocBuildSettings::BuildType::Chm:
            settings = std::make_unique<CSDocCompilerSettingsForBuildingChm>(std::move(doc_set_spec), std::move(build_settings), std::move(build_name));
            break;

        case DocBuildSettings::BuildType::Pdf:
            settings = std::make_unique<CSDocCompilerSettingsForBuildingPdf>(std::move(doc_set_spec), std::move(build_settings), std::move(build_name));
            break;

        default:
            throw ProgrammingErrorException();
    }

    if( !settings->m_buildSettings.GetOutputDirectory().empty() )
        settings->m_docSetBuildOutputDirectory = settings->GetDocSetBuildOutputDirectory();

    return settings;
}


void CSDocCompilerSettingsForBuilding::SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache)
{
    m_docSetBuilderCache = std::move(doc_set_builder_cache);
    ASSERT(m_docSetBuilderCache != nullptr);
}


std::string CSDocCompilerSettingsForBuilding::GetDocSetBuildOutputDirectoryOrFilePath(const bool directory) const
{
    ASSERT(!m_buildSettings.GetOutputDirectory().empty());

    const std::string evaluated_output_name = m_buildSettings.GetEvaluatedOutputName(*m_docSetSpec);

    if( m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::HtmlPages ||
        m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::HtmlWebsite )
    {
        ASSERT(directory);

        // if an output name is specified, or if building as part of a project, append the evaluated output name to the output directory
        if( !m_buildSettings.GetOutputName().empty() || m_docSetSpec->GetSettings().IsDocSetPartOfProject() )
            return Path::Combine(m_buildSettings.GetOutputDirectory(), evaluated_output_name);

        return m_buildSettings.GetOutputDirectory();
    }

    else
    {
        ASSERT(m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::Chm ||
               m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::Pdf);

        if( directory )
        {
            return m_buildSettings.GetOutputDirectory();
        }

        else
        {
            const char* const extension = ( m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::Chm ) ? FileExtensions::CHM :
                                                                                                                   FileExtensions::PDF;
            return PortableFunctions::CreateFilePath(m_buildSettings.GetOutputDirectory(), evaluated_output_name, extension);
        }
    }
}


const std::string& CSDocCompilerSettingsForBuilding::GetOutputDirectoryForRelativeEvaluation(const bool use_evaluated_output_directory) const
{
    // for Document Set builds
    if( !m_docSetBuildOutputDirectory.empty() )
    {
        if( use_evaluated_output_directory )
        {
            return m_docSetBuildOutputDirectory;
        }

        else
        {
            ASSERT(!m_buildSettings.GetOutputDirectory().empty());
            return m_buildSettings.GetOutputDirectory();
        }
    }

    // for CSPro Document (single file) builds
    else
    {
        ASSERT(!m_csdocOutputDirectory.empty());
        return m_csdocOutputDirectory;
    }
}


const std::string& CSDocCompilerSettingsForBuilding::GetDefaultDocumentFilePath() const
{
    return m_docSetBuilderCache->GetDefaultDocumentFilePath(*m_docSetSpec, false);
}


std::string CSDocCompilerSettingsForBuilding::CreateHtmlOutputFilePath(const std::string& csdoc_file_path) const
{
    const std::string built_file_path = GetBuiltHtmlFilePathInSourceDirectory(csdoc_file_path);
    const std::string& evaluated_output_directory = GetOutputDirectoryForRelativeEvaluation(true);

    if( m_buildSettings.BuildDocumentsUsingRelativePaths() )
    {
        // when building using relative paths, paths are based off the default document
        return CreatePathIfCopiedRelativeToFile(built_file_path, GetDefaultDocumentFilePath(), evaluated_output_directory);
    }

    else
    {
        return CreatePathIfCopiedToDirectory(built_file_path, evaluated_output_directory);
    }
}


std::string CSDocCompilerSettingsForBuilding::GetBuiltHtmlFilename(const std::string& path)
{
    return Path::AppendExtension(Path::GetFilenameWithoutExtension(path), FileExtensions::HTML);
}


std::string CSDocCompilerSettingsForBuilding::GetBuiltHtmlFilePathInSourceDirectory(const std::string& path)
{
    return PortableFunctions::PathReplaceFilename(path, GetBuiltHtmlFilename(path));
}


void CSDocCompilerSettingsForBuilding::SetOutputFilePath(std::string output_file_path)
{
    m_csdocOutputFilePath = std::move(output_file_path);
    m_csdocOutputDirectory = PortableFunctions::PathGetDirectory(m_csdocOutputFilePath);
}


std::string CSDocCompilerSettingsForBuilding::GetTitle(const std::string& csdoc_file_path)
{
    return m_docSetBuilderCache->GetTitle(m_titleManager, csdoc_file_path);
}


void CSDocCompilerSettingsForBuilding::SetTitleForCompilationFilePath(const std::string& title)
{
    return m_docSetBuilderCache->SetTitle(m_titleManager, GetCompilationFilePath(), title);
}


void CSDocCompilerSettingsForBuilding::ClearTitleForCompilationFilePath()
{
    return m_docSetBuilderCache->ClearTitle(m_titleManager, GetCompilationFilePath());
}


void CSDocCompilerSettingsForBuilding::EnsurePathIsRelative(const std::string& path)
{
    if( !Path::IsRelative(path) )
        throw CSProException("The file cannot be processed as a relative path because it is not on the same drive as the output: " + path);
}


std::string CSDocCompilerSettingsForBuilding::GetPathWithPathAdjustments(const std::string& path) const
{
    const std::tuple<std::string, std::string, bool>* best_match = nullptr;

    // find the best match, which will be the path with the longest length
    for( const std::tuple<std::string, std::string, bool>& path_and_adjustment : m_buildSettings.GetPathAdjustments() )
    {
        if( SO::StartsWithNoCase(path, std::get<0>(path_and_adjustment)) )
        {
            if( best_match == nullptr || std::get<0>(*best_match).length() < std::get<0>(path_and_adjustment).length() )
                best_match = &path_and_adjustment;
        }
    }

    if( best_match == nullptr )
        return path;

    const std::string& matched_adjustment = std::get<1>(*best_match);
    const bool& relative_to_path = std::get<2>(*best_match);

    // adjust the directory and append the rest of the path (which can include subdirectories in addition to the filename)
    if( relative_to_path )
    {
        const std::string matched_path = PortableFunctions::PathEnsureTrailingSlash(std::get<0>(*best_match));
        ASSERT(PortableFunctions::FileIsDirectory(matched_path));

        const std::string adjusted_directory = MakeFullPath(PortableFunctions::PathGetDirectory(matched_path), matched_adjustment);
        const char* const remaining_subdirectories_and_filename = path.c_str() + matched_path.length();

        return Path::Combine(adjusted_directory, remaining_subdirectories_and_filename);
    }

    // if not adjusting relative to the source path, append the filename to the matched path
    else
    {
        ASSERT(!Path::IsRelative(matched_adjustment));
        return Path::Combine(matched_adjustment, PortableFunctions::PathGetFilename(path));
    }
}


std::string CSDocCompilerSettingsForBuilding::EvaluateDirectoryRelativeToOutputDirectory(const std::string& directory, const bool use_evaluated_output_directory) const
{
    return MakeFullPath(GetOutputDirectoryForRelativeEvaluation(use_evaluated_output_directory), directory);
}


void CSDocCompilerSettingsForBuilding::WriteTextToFile(const std::string& file_path, const std::string_view text_content_sv) const
{
    if( m_docSetBuilderCache->LogWrittenFile(file_path, text_content_sv) )
        FileIO::WriteText(file_path, text_content_sv, false);
}


void CSDocCompilerSettingsForBuilding::CopyFileToDirectory(const std::string& source_file_path, const std::string& destination_file_path) const
{
    FileIO::CreateDirectoriesForFile(destination_file_path);

    std::tuple<int64_t, int64_t> file_size_and_modified_time;
    PortableFunctions::FileCopyWithExceptions(source_file_path, destination_file_path, FileOverwriteFlag::Different, &file_size_and_modified_time);

    m_docSetBuilderCache->LogCopiedFile(destination_file_path, file_size_and_modified_time);
}


std::string CSDocCompilerSettingsForBuilding::CreatePathIfCopiedToDirectory(const std::string& source_file_path, const std::string& destination_directory)
{
    return Path::Combine(destination_directory, PortableFunctions::PathGetFilename(source_file_path));
}


std::string CSDocCompilerSettingsForBuilding::CreatePathIfCopiedRelativeToFile(const std::string& source_file_path, const std::string& relative_to_file,
                                                                               const std::string& output_directory) const
{
    ASSERT(!output_directory.empty());

    const std::string adjusted_csdoc_input_path = GetPathWithPathAdjustments(relative_to_file);
    const std::string adjusted_source_file_path = GetPathWithPathAdjustments(source_file_path);

    const std::string relative_path_to_input = GetRelativePathForDisplay(adjusted_csdoc_input_path, adjusted_source_file_path);
    EnsurePathIsRelative(relative_path_to_input);

    std::string destination_file_path = MakeFullPath(output_directory, relative_path_to_input);

    ASSERT(GetRelativePathForDisplay(Path::Combine(output_directory, "fake-filename"), destination_file_path) == relative_path_to_input);

    return destination_file_path;
}


std::string CSDocCompilerSettingsForBuilding::CreatePathIfCopiedRelativeToOutput(const std::string& source_file_path) const
{
    return CreatePathIfCopiedRelativeToFile(source_file_path, GetCompilationFilePath(), m_csdocOutputDirectory);
}


std::string CSDocCompilerSettingsForBuilding::CreatePathAndCopyFileToDirectory(const std::string& source_file_path, const std::string& destination_directory) const
{
    std::string destination_file_path = CreatePathIfCopiedToDirectory(source_file_path, destination_directory);

    CopyFileToDirectory(source_file_path, destination_file_path);

    return destination_file_path;
}


std::string CSDocCompilerSettingsForBuilding::CreatePathAndCopyFileIfCopiedRelativeToOutput(const std::string& source_file_path) const
{
    std::string destination_file_path = CreatePathIfCopiedRelativeToOutput(source_file_path);

    CopyFileToDirectory(source_file_path, destination_file_path);

    return destination_file_path;
}


std::string CSDocCompilerSettingsForBuilding::CreateAbsoluteUrlForPath(std::string path)
{
    std::string url = Encoders::ToFileUrl(std::move(path));
    ASSERT(url == Encoders::ToHtmlTagValue(url));
    return url;
}


std::string CSDocCompilerSettingsForBuilding::CreateRelativeUrlForPath(const std::string& path) const
{
    ASSERT(!m_csdocOutputFilePath.empty());

    std::string relative_path_to_output = GetRelativePathForDisplay(m_csdocOutputFilePath, path);
    EnsurePathIsRelative(relative_path_to_output);

    std::string url = Encoders::ToUri(PortableFunctions::PathToForwardSlash(std::move(relative_path_to_output)));
    ASSERT(url == Encoders::ToHtmlTagValue(url));
    return url;
}


std::string CSDocCompilerSettingsForBuilding::GetStylesheetsHtml()
{
    return GetStylesheetsHtmlWorker(CSDocStylesheetFilename);
}


std::string CSDocCompilerSettingsForBuilding::GetStylesheetsHtmlWorker(const char* const css_filename) const
{
    const DocBuildSettings::StylesheetAction stylesheet_action = m_buildSettings.GetStylesheetAction();

    // embed the stylesheet...
    if( stylesheet_action == DocBuildSettings::StylesheetAction::Embed )
    {
        std::map<const char*, std::string>& embedded_stylesheets_html_cache = m_docSetBuilderCache->GetEmbeddedStylesheetsHtmlCache();

        auto lookup = embedded_stylesheets_html_cache.find(css_filename);

        if( lookup == embedded_stylesheets_html_cache.cend() )
        {
            std::string css = GetStylesheetEmbeddedHtml(FileIO::ReadText(GetStylesheetCssFilePath(css_filename)));
            lookup = embedded_stylesheets_html_cache.try_emplace(css_filename, std::move(css)).first;
        }

        return lookup->second;
    }

    else
    {
        const std::string source_css_file_path = GetStylesheetCssFilePath(css_filename);

        // ...use the existing stylesheet path
        if( stylesheet_action == DocBuildSettings::StylesheetAction::SourceAbsolute )
        {
            return GetStylesheetLinkHtml(CreateAbsoluteUrlForPath(source_css_file_path));
        }

        else if( stylesheet_action == DocBuildSettings::StylesheetAction::SourceRelative )
        {
            return GetStylesheetLinkHtml(CreateRelativeUrlForPath(source_css_file_path));
        }

        // ...or copy the stylesheet to a specific directory
        else
        {
            ASSERT(stylesheet_action == DocBuildSettings::StylesheetAction::Directory);

            const std::string stylesheet_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetStylesheetDirectory(), false);
            const std::string destination_css_file_path = CreatePathAndCopyFileToDirectory(source_css_file_path, stylesheet_directory);
            return GetStylesheetLinkHtml(CreateRelativeUrlForPath(destination_css_file_path));
        }
    }
}


std::string CSDocCompilerSettingsForBuilding::EvaluateBuildExtra(const std::string& path)
{
    std::string evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    if( !m_csdocOutputDirectory.empty() )
        CreatePathAndCopyFileToDirectory(evaluated_path, m_csdocOutputDirectory);

    return evaluated_path;
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForTitle(const std::string& path)
{
    const DocBuildSettings::TitleLinkageAction title_linkage_action = m_buildSettings.GetTitleLinkageAction();

    // suppress title links...
    if( title_linkage_action == DocBuildSettings::TitleLinkageAction::Suppress )
    {
        return std::string();
    }

    else
    {
        // ...or create a URL based on a prefix, potentially followed by the output name
        ASSERT(!m_buildSettings.GetTitleLinkPrefix().empty());

        const std::string built_filename_uri = Encoders::ToUri(GetBuiltHtmlFilename(path));

        if( title_linkage_action == DocBuildSettings::TitleLinkageAction::Prefix )
        {
            return m_buildSettings.GetTitleLinkPrefix() + built_filename_uri;
        }

        else
        {
            ASSERT(title_linkage_action == DocBuildSettings::TitleLinkageAction::OutputNamePrefix);

            const std::string evaluated_output_name = m_buildSettings.GetEvaluatedOutputName(*m_docSetSpec);
            return PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathAppendForwardSlashToPath(m_buildSettings.GetTitleLinkPrefix(),
                                                                                                                   Encoders::ToUri(evaluated_output_name)),
                                                                                                                   built_filename_uri);
        }
    }
}


const CSDocCompilerSettingsForBuilding& CSDocCompilerSettingsForBuilding::GetProjectSettings(const std::string& project) const
{
    const std::string project_doc_set_spec_file_path = m_docSetSpec->GetSettings().FindProjectDocSetSpecFilePath(project);
    return m_docSetBuilderCache->GetDocSetForProjectCompiledForDataForTree(project_doc_set_spec_file_path, *this);
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForTopic(const std::string& project, const std::string& path)
{
    if( m_docSetSpec->FindComponent(path, false) != nullptr )
    {
        if( m_buildSettings.GetDocSetLinkageAction() != DocBuildSettings::DocSetLinkageAction::Suppress )
            return CreateUrlForDocSetTopic(path);
    }

    else if( !project.empty() )
    {
        if( m_buildSettings.GetProjectLinkageAction() != DocBuildSettings::ProjectLinkageAction::Suppress )
        {
            const CSDocCompilerSettingsForBuilding& project_settings = GetProjectSettings(project);
            return CreateUrlForProjectTopic(project_settings, path);
        }
    }

    else
    {
        if( m_buildSettings.GetExternalLinkageAction() != DocBuildSettings::ExternalLinkageAction::Suppress )
            return CreateUrlForExternalTopic(path);
    }

    // suppressed links
    return std::string();
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForDocSetTopic(const std::string& path) const
{
    ASSERT(m_buildSettings.GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Link);

    // link to the built path
    const std::string built_file_path = CreateHtmlOutputFilePath(path);

    return CreateRelativeUrlForPath(built_file_path);
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    // link to the project's built path
    const std::string built_file_path = project_settings.CreateHtmlOutputFilePath(path);

    return CreateRelativeUrlForPath(built_file_path);
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForExternalTopic(const std::string& path) const
{
    const DocBuildSettings::ExternalLinkageAction external_linkage_action = m_buildSettings.GetExternalLinkageAction();
    ASSERT(external_linkage_action != DocBuildSettings::ExternalLinkageAction::Suppress);

    // issue an error for forbidden links
    if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::Forbid )
    {
        throw CSProException("The build settings forbid linking to external documents: %s", path.c_str());
    }

    // ...use a built path where the input is
    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::SourceAbsolute )
    {
        return CreateAbsoluteUrlForPath(GetBuiltHtmlFilePathInSourceDirectory(path));
    }

    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::SourceRelative )
    {
        return CreateRelativeUrlForPath(GetBuiltHtmlFilePathInSourceDirectory(path));
    }

    // ...use a built path relative to the output
    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::RelativeToOutput )
    {
        const std::string built_file_path_in_destination_directory = CreatePathIfCopiedRelativeToOutput(GetBuiltHtmlFilePathInSourceDirectory(path));
        return CreateRelativeUrlForPath(built_file_path_in_destination_directory);
    }

    // ...use a built path in a specific directory
    else
    {
        ASSERT(external_linkage_action == DocBuildSettings::ExternalLinkageAction::Directory);

        const std::string external_link_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetExternalLinkDirectory(), false);
        const std::string built_file_path_in_destination_directory = CreatePathIfCopiedToDirectory(GetBuiltHtmlFilename(path), external_link_directory);
        return CreateRelativeUrlForPath(built_file_path_in_destination_directory);
    }
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForLogicTopic(const char* const help_topic_filename)
{
    const DocBuildSettings::LogicLinkageAction logic_linkage_action = m_buildSettings.GetLogicLinkageAction();

    return ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::Suppress )   ? std::string() :
           ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::CSProUsers ) ? CreateUrlForLogicTopicOnCSProUsersWebsite(help_topic_filename) :
           ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::Project )    ? CreateUrlForLogicHelpTopicInCSProProject(help_topic_filename) :
                                                                                          throw ProgrammingErrorException();
}


std::string CSDocCompilerSettingsForBuilding::CreateUrlForImageFile(const std::string& path)
{
    const DocBuildSettings::ImageAction image_action = m_buildSettings.GetImageAction();

    // embed the image as a data URL...
    if( image_action == DocBuildSettings::ImageAction::DataUrl )
    {
        return CSDocCompilerSettings::CreateUrlForImageFile(path);
    }

    // ...use the existing image path
    else if( image_action == DocBuildSettings::ImageAction::SourceAbsolute )
    {
        return CreateAbsoluteUrlForPath(path);
    }

    else if( image_action == DocBuildSettings::ImageAction::SourceRelative )
    {
        return CreateRelativeUrlForPath(path);
    }

    // ...copy the image to a directory relative to the output
    else if( image_action == DocBuildSettings::ImageAction::RelativeToOutput )
    {
        const std::string destination_image_path = CreatePathAndCopyFileIfCopiedRelativeToOutput(path);
        return CreateRelativeUrlForPath(destination_image_path);
    }

    // ...or copy the image to a specific directory
    else
    {
        ASSERT(image_action == DocBuildSettings::ImageAction::Directory);

        const std::string image_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetImageDirectory(), false);
        const std::string destination_image_path = CreatePathAndCopyFileToDirectory(path, image_directory);
        return CreateRelativeUrlForPath(destination_image_path);
    }
}
