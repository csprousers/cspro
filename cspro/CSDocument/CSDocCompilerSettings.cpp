#include "StdAfx.h"
#include "CSDocCompilerSettings.h"
#include "SearchFilePathsByFilename.h"


CSDocCompilerSettings::CSDocCompilerSettings(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   m_docSetSpec(std::move(doc_set_spec)),
        m_titleManager(m_docSetSpec.get())
{
}


const std::string& CSDocCompilerSettings::GetCompilationFilePath() const
{
    return !m_compilationFilePaths.empty() ? m_compilationFilePaths.back() :
                                             ReturnProgrammingError(SO::Empty_string);
}


RAII::PushOnVectorAndPopOnDestruction<std::string> CSDocCompilerSettings::SetCompilationFilePath(std::string csdoc_file_path)
{
    ASSERT(csdoc_file_path.empty() || PortableFunctions::FileIsRegular(csdoc_file_path));

    return RAII::PushOnVectorAndPopOnDestruction<std::string>(m_compilationFilePaths, std::move(csdoc_file_path));
}


const std::string& CSDocCompilerSettings::GetDefinition(const std::string& key) const
{
    const std::vector<std::tuple<std::string, std::string>>& definitions = m_docSetSpec->GetDefinitions();

    const auto& key_lookup = std::find_if(definitions.begin(), definitions.end(),
                                          [&](const std::tuple<std::string, std::string>& key_and_value) { return ( std::get<0>(key_and_value) == key ); });

    if( key_lookup == definitions.cend() )
        throw CSProException("The definition '%s' does not exist.", key.c_str());

    return std::get<1>(*key_lookup);
}


std::string CSDocCompilerSettings::GetSpecialDefinition(const std::string& domain, const std::string& key) const
{
    if( domain == "DocumentSet" )
    {
        if( key == "title" )
        {
            if( !m_docSetSpec->GetTitle().has_value() )
                throw CSProException("The Document Set does not have a defined title.");

            return *m_docSetSpec->GetTitle();
        }
    }

    else if( domain == "CSPro" )
    {
        if( key == "version" )
            return Versioning::NumberText;
    }

    else if( domain == "System" )
    {
        if( key == "year" )
            return IntToString(DateTime::LocalYear());
    }

    else
    {
        throw CSProException("The special definition domain '%s' is not known.", domain.c_str());
    }

    throw CSProException("The special definition '%s' does not exist in the domain '%s'.", key.c_str(), domain.c_str());
}


void CSDocCompilerSettings::AddCompilerMessage(CompilerMessageType /*compiler_message_type*/, const std::string& /*text*/)
{
}


std::string CSDocCompilerSettings::GetStylesheetCssFilePath(const char* const css_filename)
{
    return Path::Combine(Html::GetDirectory(Html::Subdirectory::Document), css_filename);
}


std::string CSDocCompilerSettings::GetStylesheetLinkHtml(const std::string& css_url)
{
    return SO::Concatenate("<link href=\"",
                           Encoders::ToHtmlTagValue(css_url),
                           "\" rel=\"stylesheet\" type=\"text/css\" />\n");
}


std::string CSDocCompilerSettings::GetStylesheetEmbeddedHtml(std::string css)
{
    // remove \r from the CSS
    SO::MakeNewlineLF(css);

    return SO::Concatenate("<style>\n",
                           css,
                           "</style>\n");
}


std::string CSDocCompilerSettings::GetStylesheetsHtml()
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed);

    static const std::string embedded_stylesheets_html = GetStylesheetEmbeddedHtml(FileIO::ReadText(GetStylesheetCssFilePath(CSDocStylesheetFilename)));
    return embedded_stylesheets_html;
}


std::string CSDocCompilerSettings::EvaluatePath(std::string path) const
{
    // if the path starts with a slash, evaluate it based on the project root
    if( !path.empty() && Path::IsSlashChar(path.front()) )
    {
        const std::string& project_root_directory = m_docSetSpec->GetSettings().GetProjectRootDirectory();

        if( project_root_directory.empty() )
            throw CSProException("No project root directory has been set so a path cannot be evaluated from the root: " + path);

        path.erase(0, 1);

        path = MakeFullPath(project_root_directory, std::move(path));
    }

    // otherwise evaluate the path based on the current document's directory
    else if( !m_compilationFilePaths.empty() )
    {
        path = MakeFullPath(PortableFunctions::PathGetDirectory(m_compilationFilePaths.back()), std::move(path));
    }

    return CheckPathCase(std::move(path));
}


std::string CSDocCompilerSettings::EvaluateTopicPath(const std::string& path)
{
    std::string evaluated_path = EvaluatePath(path);

    if( PortableFunctions::FileIsRegular(evaluated_path) )
        return evaluated_path;

    // if the file does not exist, see if there are any documents with the name
    const std::vector<std::shared_ptr<DocSetComponent>>* const doc_set_components_with_name = m_docSetSpec->FindDocument(path);

    if( doc_set_components_with_name != nullptr )
    {
        ASSERT(doc_set_components_with_name->size() >= 1);

        if( doc_set_components_with_name->size() > 1 )
        {
            throw SearchFilePathsByFilename::AmbiguousException(path, doc_set_components_with_name->front()->file_path,
                                                                      doc_set_components_with_name->at(1)->file_path);
        }

        return CheckPathCase(doc_set_components_with_name->front()->file_path, path);
    }

    return evaluated_path;
}


std::string CSDocCompilerSettings::EvaluateTopicPath(const std::string& project, const std::string& path)
{
    try
    {
        const std::string project_doc_set_spec_file_path = m_docSetSpec->GetSettings().FindProjectDocSetSpecFilePath(project);

        // this may be an internal link
        if( SO::EqualsNoCase(project_doc_set_spec_file_path, m_docSetSpec->GetFilePath()) )
            return EvaluateTopicPath(path);

        return CheckPathCase(SearchFilePathsByFilename::Search(CacheableCalculator::GetDocumentFilePathsForProject(project_doc_set_spec_file_path), path), path);
    }

    catch( const SearchFilePathsByFilename::AmbiguousException& ambiguous_exception )
    {
        throw CSProException("The path '%s::%s' is ambiguous. It could refer to '%s' or '%s'.",
                             project.c_str(), path.c_str(), ambiguous_exception.path1.c_str(), ambiguous_exception.path2.c_str());
    }

    catch( const SearchFilePathsByFilename::NotFoundException& )
    {
        throw CSProException("The path '%s' does not exist in the project: %s", path.c_str(), project.c_str());
    }

    catch( const CSProException& exception )
    {
        throw CSProException("There was an error processing the project: %s: %s", project.c_str(), exception.what());
    }
}


std::string CSDocCompilerSettings::EvaluateImagePath(const std::string& path)
{
    std::string evaluated_path = EvaluatePath(path);

    if( PortableFunctions::FileIsRegular(evaluated_path) )
        return evaluated_path;

    // if the file does not exist, look in any specified image directories
    const std::string* const image_with_name_path = m_docSetSpec->GetSettings().FindInImageDirectories(path);

    return ( image_with_name_path != nullptr ) ? CheckPathCase(*image_with_name_path, path) :
                                                 evaluated_path;
}


std::string CSDocCompilerSettings::EvaluateBuildExtra(const std::string& path)
{
    std::string evaluated_path = EvaluatePath(path);

    if( !PortableFunctions::FileIsRegular(evaluated_path) )
        throw CSProException("The build extra could not be located: %s", evaluated_path.c_str());

    return evaluated_path;
}


std::string CSDocCompilerSettings::CreateUrlForTitle(const std::string& /*path*/)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetTitleLinkageAction() == DocBuildSettings::TitleLinkageAction::Suppress);

    return std::string();
}


std::string CSDocCompilerSettings::CreateUrlForTopic(const std::string& /*project*/, const std::string& /*path*/)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || ( GetBuildSettingsDebug()->GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Suppress &&
                                                   GetBuildSettingsDebug()->GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Suppress &&
                                                   GetBuildSettingsDebug()->GetExternalLinkageAction() == DocBuildSettings::ExternalLinkageAction::Suppress) );

    return std::string();
}


std::string CSDocCompilerSettings::CreateUrlForLogicTopic(const char* const help_topic_filename)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetLogicLinkageAction() == DocBuildSettings::LogicLinkageAction::CSProUsers);

    return CreateUrlForLogicTopicOnCSProUsersWebsite(help_topic_filename);
}


std::string CSDocCompilerSettings::CreateUrlForLogicTopicOnCSProUsersWebsite(const char* const help_topic_filename)
{
    ASSERT(PortableFunctions::PathGetFileExtension(help_topic_filename) == FileExtensions::HTML);

    return "https://csprousers.org/help/CSPro/" + Encoders::ToUri(help_topic_filename);
}


std::string CSDocCompilerSettings::CreateUrlForLogicHelpTopicInCSProProject(const char* const help_topic_filename)
{
    const std::string csdoc_file_path = PortableFunctions::PathReplaceFileExtension(help_topic_filename, FileExtensions::CSDocument);
    const std::string project = "CSPro";
    const std::string path = EvaluateTopicPath(project, csdoc_file_path);

    return CreateUrlForTopic(project, path);
}


std::string CSDocCompilerSettings::CreateUrlForImageFile(const std::string& path)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetImageAction() == DocBuildSettings::ImageAction::DataUrl);

    return Encoders::ToDataUrl(*FileIO::Read(path),
                               ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path))));
}


std::optional<unsigned> CSDocCompilerSettings::GetContextId(const std::string& context, bool /*use_if_exists*/)
{
    const std::map<std::string, unsigned>& context_ids = m_docSetSpec->GetContextIds();
    const auto& lookup = context_ids.find(context);

    if( lookup != context_ids.cend() )
        return lookup->second;

    return std::nullopt;
}


std::string CSDocCompilerSettings::CheckPathCase(std::string path, const std::string& specified_case_to_check/* = SO::Empty_string*/)
{
#ifdef CHECK_PATH_CASE
    wchar_t short_path[MAX_PATH];

    if( GetShortPathName(TC::ToWide(path).c_str(), short_path, _countof(short_path)) > 0 )
    {
        wchar_t long_path[MAX_PATH];

        if( GetLongPathName(short_path, long_path, _countof(long_path)) > 0 )
        {
            if( !SO::Equals(path, long_path) )
                throw CSProException("The path '%s' must be used as exists on disk: '%s'", path.c_str(), TC::ToUtf8(long_path).c_str());

            if( !specified_case_to_check.empty() && path.find(specified_case_to_check) == std::string::npos )
                throw CSProException("The path '%s' must be used as exists on disk: '%s'", specified_case_to_check.c_str(), TC::ToUtf8(long_path).c_str());
        }
    }

#else
    specified_case_to_check;

#endif

    return path;
}
