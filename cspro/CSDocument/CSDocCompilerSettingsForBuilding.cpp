#include "StdAfx.h"
#include "CSDocCompilerSettings.h"
#include "DocSetBuilderCache.h"


CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                   std::wstring build_name)
    :   CSDocCompilerSettings(std::move(doc_set_spec)),
        m_buildSettings(std::move(build_settings)),
        m_buildName(std::move(build_name)),
        m_docSetBuilderCache(std::make_shared<DocSetBuilderCache>())
{
}


std::unique_ptr<CSDocCompilerSettingsForBuilding>
CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, const DocBuildSettings& base_build_settings,
                                                       DocBuildSettings::BuildType build_type, std::wstring build_name,
                                                       bool throw_exceptions_for_serious_issues_when_validating_build_settings)
{
    // create build settings for the build type and apply any user build settings on top
    DocBuildSettings build_settings = DocBuildSettings::ApplySettings(DocBuildSettings::DefaultSettingsForBuildType(build_type), base_build_settings);

    // make sure the build type is properly set (in case it was overriden by ApplySettings)
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


std::wstring CSDocCompilerSettingsForBuilding::GetDocSetBuildOutputDirectoryOrFilename(bool directory) const
{
    ASSERT(!m_buildSettings.GetOutputDirectory().empty());

    const std::wstring evaluated_output_name = m_buildSettings.GetEvaluatedOutputName(*m_docSetSpec);

    if( m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::HtmlPages ||
        m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::HtmlWebsite )
    {
        ASSERT(directory);

        // if an output name is specified, or if building as part of a project, append the evaluated output name to the output directory
        if( !m_buildSettings.GetOutputName().empty() || m_docSetSpec->GetSettings().IsDocSetPartOfProject() )
            return PortableFunctions::PathAppendToPath(m_buildSettings.GetOutputDirectory(), evaluated_output_name);

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
            const TCHAR* const extension = ( m_buildSettings.GetBuildType() == DocBuildSettings::BuildType::Chm ) ? FileExtensions::WithDot::CHM :
                                                                                                                    FileExtensions::WithDot::PDF;

            return PortableFunctions::PathAppendToPath(m_buildSettings.GetOutputDirectory(), evaluated_output_name + extension);
        }
    }
}


const std::wstring& CSDocCompilerSettingsForBuilding::GetOutputDirectoryForRelativeEvaluation(bool use_evaluated_output_directory) const
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


const std::wstring& CSDocCompilerSettingsForBuilding::GetDefaultDocumentPath() const
{
    return m_docSetBuilderCache->GetDefaultDocumentPath(*m_docSetSpec, false);
}


std::wstring CSDocCompilerSettingsForBuilding::CreateHtmlOutputFilename(const std::wstring& csdoc_filename) const
{
    const std::wstring built_path = GetBuiltHtmlPathInSourceDirectory(csdoc_filename);
    const std::wstring& evaluated_output_directory = GetOutputDirectoryForRelativeEvaluation(true);

    if( m_buildSettings.BuildDocumentsUsingRelativePaths() )
    {
        // when building using relative paths, paths are based off the default document
        return CreatePathIfCopiedRelativeToFile(built_path, GetDefaultDocumentPath(), evaluated_output_directory);
    }

    else
    {
        return CreatePathIfCopiedToDirectory(built_path, evaluated_output_directory);
    }
}


std::wstring CSDocCompilerSettingsForBuilding::GetBuiltHtmlFilename(const std::wstring& path)
{
    return PortableFunctions::PathGetFilenameWithoutExtension(path) + FileExtensions::WithDot::HTML;
}


std::wstring CSDocCompilerSettingsForBuilding::GetBuiltHtmlPathInSourceDirectory(const std::wstring& path)
{
    return PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory(path), GetBuiltHtmlFilename(path));
}


void CSDocCompilerSettingsForBuilding::SetOutputFilename(std::wstring output_filename)
{
    m_csdocOutputFilename = std::move(output_filename);
    m_csdocOutputDirectory = PortableFunctions::PathGetDirectory(m_csdocOutputFilename);
}


std::wstring CSDocCompilerSettingsForBuilding::GetTitle(const std::wstring& csdoc_filename)
{
    return m_docSetBuilderCache->GetTitle(m_titleManager, csdoc_filename);
}


void CSDocCompilerSettingsForBuilding::SetTitleForCompilationFilename(const std::wstring& title)
{
    return m_docSetBuilderCache->SetTitle(m_titleManager, GetCompilationFilename(), title);
}


void CSDocCompilerSettingsForBuilding::ClearTitleForCompilationFilename()
{
    return m_docSetBuilderCache->ClearTitle(m_titleManager, GetCompilationFilename());
}


void CSDocCompilerSettingsForBuilding::EnsurePathIsRelative(const std::wstring& path)
{
    if( !PathIsRelative(path.c_str()) )
        throw CSProException(_T("The file cannot be processed as a relative path because it is not on the same drive as the output: ") + path);
}


std::wstring CSDocCompilerSettingsForBuilding::GetPathWithPathAdjustments(const std::wstring& path) const
{
    const std::tuple<std::wstring, std::wstring, bool>* best_match = nullptr;

    // find the best match, which will be the path with the longest length
    for( const std::tuple<std::wstring, std::wstring, bool>& path_and_adjustment : m_buildSettings.GetPathAdjustments() )
    {
        if( SO::StartsWithNoCase(path, std::get<0>(path_and_adjustment)) )
        {
            if( best_match == nullptr || std::get<0>(*best_match).length() < std::get<0>(path_and_adjustment).length() )
                best_match = &path_and_adjustment;
        }
    }

    if( best_match == nullptr )
        return path;

    const std::wstring& matched_adjustment = std::get<1>(*best_match);
    const bool& relative_to_path = std::get<2>(*best_match);

    // adjust the directory and append the rest of the path (which can include subdirectories in addition to the filename)
    if( relative_to_path )
    {
        const std::wstring matched_path = PortableFunctions::PathEnsureTrailingSlash(std::get<0>(*best_match));
        ASSERT(PortableFunctions::FileIsDirectory(matched_path));

        const std::wstring adjusted_directory = MakeFullPath(PortableFunctions::PathGetDirectory(matched_path), matched_adjustment);
        const TCHAR* remaining_subdirectories_and_filename = path.c_str() + matched_path.length();

        return PortableFunctions::PathAppendToPath(adjusted_directory, remaining_subdirectories_and_filename);
    }

    // if not adjusting relative to the source path, append the filename to the matched path
    else
    {
        ASSERT(!PathIsRelative(matched_adjustment.c_str()));
        return PortableFunctions::PathAppendToPath(matched_adjustment, PortableFunctions::PathGetFilename(path));
    }
}


std::wstring CSDocCompilerSettingsForBuilding::EvaluateDirectoryRelativeToOutputDirectory(const std::wstring& directory, bool use_evaluated_output_directory) const
{
    return MakeFullPath(GetOutputDirectoryForRelativeEvaluation(use_evaluated_output_directory), directory);
}


void CSDocCompilerSettingsForBuilding::WriteTextToFile(const std::wstring& filename, wstring_view text_content_sv, bool write_utf8_bom) const
{
    if( m_docSetBuilderCache->LogWrittenFile(filename, text_content_sv) )
        FileIO::WriteText(filename, text_content_sv, write_utf8_bom);
}


void CSDocCompilerSettingsForBuilding::CopyFileToDirectory(const std::wstring& source_path, const std::wstring& destination_path) const
{
    FileIO::CreateDirectoriesForFile(destination_path);

    std::tuple<int64_t, time_t> file_size_and_modified_time;
    PortableFunctions::FileCopyWithExceptions(source_path, destination_path, PortableFunctions::FileCopyType::CopyIfDifferent, &file_size_and_modified_time);

    m_docSetBuilderCache->LogCopiedFile(destination_path, file_size_and_modified_time);
}


std::wstring CSDocCompilerSettingsForBuilding::CreatePathIfCopiedToDirectory(const std::wstring& source_path, const std::wstring& destination_directory)
{
    return PortableFunctions::PathAppendToPath(destination_directory, PortableFunctions::PathGetFilename(source_path));
}


std::wstring CSDocCompilerSettingsForBuilding::CreatePathIfCopiedRelativeToFile(const std::wstring& source_path, const std::wstring& relative_to_file,
                                                                                const std::wstring& output_directory) const
{
    ASSERT(!output_directory.empty());

    const std::wstring adjusted_csdoc_input_path = GetPathWithPathAdjustments(relative_to_file);
    const std::wstring adjusted_source_path = GetPathWithPathAdjustments(source_path);

    const std::wstring relative_path_to_input = GetRelativeFNameForDisplay(adjusted_csdoc_input_path, adjusted_source_path);
    EnsurePathIsRelative(relative_path_to_input);

    std::wstring destination_path = MakeFullPath(output_directory, relative_path_to_input);

    ASSERT(GetRelativeFNameForDisplay(PortableFunctions::PathAppendToPath(output_directory, _T("fake-filename")), destination_path) == relative_path_to_input);

    return destination_path;
}


std::wstring CSDocCompilerSettingsForBuilding::CreatePathIfCopiedRelativeToOutput(const std::wstring& source_path) const
{
    return CreatePathIfCopiedRelativeToFile(source_path, GetCompilationFilename(), m_csdocOutputDirectory);
}


std::wstring CSDocCompilerSettingsForBuilding::CreatePathAndCopyFileToDirectory(const std::wstring& source_path, const std::wstring& destination_directory) const
{
    std::wstring destination_path = CreatePathIfCopiedToDirectory(source_path, destination_directory);

    CopyFileToDirectory(source_path, destination_path);

    return destination_path;
}


std::wstring CSDocCompilerSettingsForBuilding::CreatePathAndCopyFileIfCopiedRelativeToOutput(const std::wstring& source_path) const
{
    std::wstring destination_path = CreatePathIfCopiedRelativeToOutput(source_path);

    CopyFileToDirectory(source_path, destination_path);

    return destination_path;
}


std::wstring CSDocCompilerSettingsForBuilding::CreateAbsoluteUrlForPath(const std::wstring& path)
{
    std::wstring url = Encoders::ToFileUrl(path);
    ASSERT(url == Encoders::ToHtmlTagValue(url));
    return url;
}


std::wstring CSDocCompilerSettingsForBuilding::CreateRelativeUrlForPath(const std::wstring& path) const
{
    ASSERT(!m_csdocOutputFilename.empty());

    std::wstring relative_path_to_output = GetRelativeFNameForDisplay(m_csdocOutputFilename, path);
    EnsurePathIsRelative(relative_path_to_output);

    std::wstring url = Encoders::ToUri(PortableFunctions::PathToForwardSlash(std::move(relative_path_to_output)));
    ASSERT(url == Encoders::ToHtmlTagValue(url));
    return url;
}


std::wstring CSDocCompilerSettingsForBuilding::GetStylesheetsHtml()
{
    return GetStylesheetsHtmlWorker(CSDocStylesheetFilename);
}


std::wstring CSDocCompilerSettingsForBuilding::GetStylesheetsHtmlWorker(const TCHAR* css_filename) const
{
    const DocBuildSettings::StylesheetAction stylesheet_action = m_buildSettings.GetStylesheetAction();

    // embed the stylesheet...
    if( stylesheet_action == DocBuildSettings::StylesheetAction::Embed )
    {
        std::map<const TCHAR*, std::wstring>& embedded_stylesheets_html_cache = m_docSetBuilderCache->GetEmbeddedStylesheetsHtmlCache();

        auto lookup = embedded_stylesheets_html_cache.find(css_filename);

        if( lookup == embedded_stylesheets_html_cache.cend() )
        {
            std::wstring css = GetStylesheetEmbeddedHtml(FileIO::ReadText(GetStylesheetCssPath(css_filename)));
            lookup = embedded_stylesheets_html_cache.try_emplace(css_filename, std::move(css)).first;
        }

        return lookup->second;
    }

    else 
    {
        const std::wstring source_css_path = GetStylesheetCssPath(css_filename);

        // ...use the existing stylesheet path
        if( stylesheet_action == DocBuildSettings::StylesheetAction::SourceAbsolute )
        {
            return GetStylesheetLinkHtml(CreateAbsoluteUrlForPath(source_css_path));
        }

        else if( stylesheet_action == DocBuildSettings::StylesheetAction::SourceRelative )
        {
            return GetStylesheetLinkHtml(CreateRelativeUrlForPath(source_css_path));
        }

        // ...or copy the stylesheet to a specific directory
        else 
        {
            ASSERT(stylesheet_action == DocBuildSettings::StylesheetAction::Directory);

            const std::wstring stylesheet_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetStylesheetDirectory(), false);
            const std::wstring destination_css_path = CreatePathAndCopyFileToDirectory(source_css_path, stylesheet_directory);
            return GetStylesheetLinkHtml(CreateRelativeUrlForPath(destination_css_path));
        }
    }
}


std::wstring CSDocCompilerSettingsForBuilding::EvaluateBuildExtra(const std::wstring& path)
{
    std::wstring evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    if( !m_csdocOutputDirectory.empty() )
        CreatePathAndCopyFileToDirectory(evaluated_path, m_csdocOutputDirectory);

    return evaluated_path;
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForTitle(const std::wstring& path)
{
    const DocBuildSettings::TitleLinkageAction title_linkage_action = m_buildSettings.GetTitleLinkageAction();

    // suppress title links...
    if( title_linkage_action == DocBuildSettings::TitleLinkageAction::Suppress )
    {
        return std::wstring();
    }

    else
    {
        // ...or create a URL based on a prefix, potentially followed by the output name
        ASSERT(!m_buildSettings.GetTitleLinkPrefix().empty());

        const std::wstring built_filename_uri = Encoders::ToUri(GetBuiltHtmlFilename(path));

        if( title_linkage_action == DocBuildSettings::TitleLinkageAction::Prefix )
        {
            return m_buildSettings.GetTitleLinkPrefix() + built_filename_uri;
        }

        else
        {
            ASSERT(title_linkage_action == DocBuildSettings::TitleLinkageAction::OutputNamePrefix);

            const std::wstring evaluated_output_name = m_buildSettings.GetEvaluatedOutputName(*m_docSetSpec);
            return PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathAppendForwardSlashToPath(m_buildSettings.GetTitleLinkPrefix(),
                                                                                                                   Encoders::ToUri(evaluated_output_name)),
                                                                                                                   built_filename_uri);
        }
    }
}


const CSDocCompilerSettingsForBuilding& CSDocCompilerSettingsForBuilding::GetProjectSettings(const std::wstring& project) const
{
    const std::wstring project_doc_set_spec_filename = m_docSetSpec->GetSettings().FindProjectDocSetSpecFilename(project);
    return m_docSetBuilderCache->GetDocSetForProjectCompiledForDataForTree(project_doc_set_spec_filename, *this);
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForTopic(const std::wstring& project, const std::wstring& path)
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
    return std::wstring();
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForDocSetTopic(const std::wstring& path) const
{
    ASSERT(m_buildSettings.GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Link);

    // link to the built path
    const std::wstring built_path = CreateHtmlOutputFilename(path);

    return CreateRelativeUrlForPath(built_path);
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    // link to the project's built path
    const std::wstring built_path = project_settings.CreateHtmlOutputFilename(path);

    return CreateRelativeUrlForPath(built_path);
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForExternalTopic(const std::wstring& path) const
{
    const DocBuildSettings::ExternalLinkageAction external_linkage_action = m_buildSettings.GetExternalLinkageAction();
    ASSERT(external_linkage_action != DocBuildSettings::ExternalLinkageAction::Suppress);

    // issue an error for forbidden links
    if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::Forbid )
    {
        throw CSProException(_T("The build settings forbid linking to external documents: ") + path);
    }

    // ...use a built path where the input is
    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::SourceAbsolute )
    {
        return CreateAbsoluteUrlForPath(GetBuiltHtmlPathInSourceDirectory(path));
    }

    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::SourceRelative )
    {
        return CreateRelativeUrlForPath(GetBuiltHtmlPathInSourceDirectory(path));
    }

    // ...use a built path relative to the output
    else if( external_linkage_action == DocBuildSettings::ExternalLinkageAction::RelativeToOutput )
    {
        const std::wstring built_path_in_destination_directory = CreatePathIfCopiedRelativeToOutput(GetBuiltHtmlPathInSourceDirectory(path));
        return CreateRelativeUrlForPath(built_path_in_destination_directory);
    }

    // ...use a built path in a specific directory
    else
    {
        ASSERT(external_linkage_action == DocBuildSettings::ExternalLinkageAction::Directory);

        const std::wstring external_link_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetExternalLinkDirectory(), false);
        const std::wstring built_path_in_destination_directory = CreatePathIfCopiedToDirectory(GetBuiltHtmlFilename(path), external_link_directory);
        return CreateRelativeUrlForPath(built_path_in_destination_directory);
    }
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForLogicTopic(const TCHAR* help_topic_filename)
{
    const DocBuildSettings::LogicLinkageAction logic_linkage_action = m_buildSettings.GetLogicLinkageAction();

    return ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::Suppress )   ? std::wstring() :
           ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::CSProUsers ) ? CreateUrlForLogicTopicOnCSProUsersForum(help_topic_filename) :
           ( logic_linkage_action == DocBuildSettings::LogicLinkageAction::Project )    ? CreateUrlForLogicHelpTopicInCSProProject(help_topic_filename) :
                                                                                          throw ProgrammingErrorException();
}


std::wstring CSDocCompilerSettingsForBuilding::CreateUrlForImageFile(const std::wstring& path)
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
        const std::wstring destination_image_path = CreatePathAndCopyFileIfCopiedRelativeToOutput(path);
        return CreateRelativeUrlForPath(destination_image_path);
    }

    // ...or copy the image to a specific directory
    else
    {
        ASSERT(image_action == DocBuildSettings::ImageAction::Directory);

        const std::wstring image_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetImageDirectory(), false);
        const std::wstring destination_image_path = CreatePathAndCopyFileToDirectory(path, image_directory);
        return CreateRelativeUrlForPath(destination_image_path);
    }
}
