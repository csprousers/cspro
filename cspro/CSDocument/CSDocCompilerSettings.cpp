#include "StdAfx.h"
#include "CSDocCompilerSettings.h"
#include "SearchFilenamesByName.h"


namespace
{
    std::wstring CheckPathCase(std::wstring path, const std::wstring& specified_case_to_check = std::wstring())
    {
#ifdef CHECK_PATH_CASE
        TCHAR short_path[MAX_PATH];

        if( GetShortPathName(path.c_str(), short_path, _countof(short_path)) > 0 )
        {
            TCHAR long_path[MAX_PATH];

            if( GetLongPathName(short_path, long_path, _countof(long_path)) > 0 )
            {
                if( !SO::Equals(path, long_path) )
                    throw CSProException(_T("The path '%s' must be used as exists on disk: '%s'"), path.c_str(), long_path);

                if( !specified_case_to_check.empty() && path.find(specified_case_to_check) == std::wstring::npos )
                    throw CSProException(_T("The path '%s' must be used as exists on disk: '%s'"), specified_case_to_check.c_str(), long_path);
            }
        }

#else
        specified_case_to_check;

#endif

        return path;
    }
}


CSDocCompilerSettings::CSDocCompilerSettings(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   m_docSetSpec(std::move(doc_set_spec)),
        m_titleManager(m_docSetSpec.get())
{
}


const std::wstring& CSDocCompilerSettings::GetCompilationFilename() const
{
    return !m_compilationFilenames.empty() ? m_compilationFilenames.back() :
                                             ReturnProgrammingError(SO::EmptyString);
}


RAII::PushOnVectorAndPopOnDestruction<std::wstring> CSDocCompilerSettings::SetCompilationFilename(std::wstring csdoc_filename)
{
    ASSERT(csdoc_filename.empty() || PortableFunctions::FileIsRegular(csdoc_filename));

    return RAII::PushOnVectorAndPopOnDestruction<std::wstring>(m_compilationFilenames, std::move(csdoc_filename));
}


const std::wstring& CSDocCompilerSettings::GetDefinition(const std::wstring& key) const
{
    const std::vector<std::tuple<std::wstring, std::wstring>>& definitions = m_docSetSpec->GetDefinitions();

    const auto& key_lookup = std::find_if(definitions.begin(), definitions.end(),
        [&](const std::tuple<std::wstring, std::wstring>& key_and_value) { return ( std::get<0>(key_and_value) == key ); });

    if( key_lookup == definitions.cend() )
        throw CSProException(_T("The definition '%s' does not exist."), key.c_str());

    return std::get<1>(*key_lookup);    
}


std::wstring CSDocCompilerSettings::GetSpecialDefinition(const std::wstring& domain, const std::wstring& key) const
{
    if( domain == _T("DocumentSet") )
    {
        if( key == _T("title") )
        {
            if( !m_docSetSpec->GetTitle().has_value() )
                throw CSProException("The Document Set does not have a defined title.");

            return *m_docSetSpec->GetTitle();            
        }            
    }

    else if( domain == _T("CSPro") )
    {
        if( key == _T("version") )
            return CSPRO_VERSION_NUMBER_TEXT;
    }

    else if( domain == _T("System") )
    {
        if( key == _T("year") )
        {
            struct tm tp = GetLocalTime();
            return CS2WS(IntToString(tp.tm_year + 1900));
        }            
    }

    else
    {
        throw CSProException(_T("The special definition domain '%s' is not known."), domain.c_str());
    }

    throw CSProException(_T("The special definition '%s' does not exist in the domain '%s'."), key.c_str(), domain.c_str());
}


void CSDocCompilerSettings::AddCompilerMessage(CompilerMessageType /*compiler_message_type*/, const std::wstring& /*text*/)
{
}


std::wstring CSDocCompilerSettings::GetStylesheetCssPath(const TCHAR* css_filename)
{
    return CheckPathCase(PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Document), css_filename));
}


std::wstring CSDocCompilerSettings::GetStylesheetLinkHtml(const std::wstring& css_url)
{
    return _T("<link href=\"") + Encoders::ToHtmlTagValue(css_url) + _T("\" rel=\"stylesheet\" type=\"text/css\" />\n");
}


std::wstring CSDocCompilerSettings::GetStylesheetEmbeddedHtml(std::wstring css)
{
    // remove \r from the CSS
    SO::MakeNewlineLF(css);
    return _T("<style>\n") + css + _T("</style>\n");
}


std::wstring CSDocCompilerSettings::GetStylesheetsHtml()
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed);

    static std::wstring embedded_stylesheets_html = GetStylesheetEmbeddedHtml(FileIO::ReadText(GetStylesheetCssPath(CSDocStylesheetFilename)));
    return embedded_stylesheets_html;
}


std::wstring CSDocCompilerSettings::EvaluatePath(std::wstring path) const
{
    // evaluate the path based on the document's directory
    if( !m_compilationFilenames.empty() )
        return CheckPathCase(MakeFullPath(PortableFunctions::PathGetDirectory(m_compilationFilenames.back()), std::move(path)));

    return CheckPathCase(path);
}


std::wstring CSDocCompilerSettings::EvaluateTopicPath(const std::wstring& path)
{
    std::wstring evaluated_path = EvaluatePath(path);

    if( PortableFunctions::FileIsRegular(evaluated_path) )
        return evaluated_path;

    // if the file does not exist, see if there are any documents with the name
    const std::vector<std::shared_ptr<DocSetComponent>>* doc_set_components_with_name = m_docSetSpec->FindDocument(path);

    if( doc_set_components_with_name != nullptr )
    {
        ASSERT(doc_set_components_with_name->size() >= 1);

        if( doc_set_components_with_name->size() > 1 )
        {
            throw SearchFilenamesByName::AmbiguousException(path, doc_set_components_with_name->front()->filename,
                                                                  doc_set_components_with_name->at(1)->filename);
        }

        return CheckPathCase(doc_set_components_with_name->front()->filename, path);
    }

    return evaluated_path;
}


std::wstring CSDocCompilerSettings::EvaluateTopicPath(const std::wstring& project, const std::wstring& path)
{
    try
    {
        const std::wstring project_doc_set_spec_filename = m_docSetSpec->GetSettings().FindProjectDocSetSpecFilename(project);

        // this may be an internal link
        if( SO::EqualsNoCase(project_doc_set_spec_filename, m_docSetSpec->GetFilename()) )
            return EvaluateTopicPath(path);

        return CheckPathCase(SearchFilenamesByName::Search(CacheableCalculator::GetDocumentFilenamesForProject(project_doc_set_spec_filename), path), path);
    }

    catch( const SearchFilenamesByName::AmbiguousException& ambiguous_exception )
    {
        throw CSProException(_T("The path '%s::%s' is ambiguous. It could refer to '%s' or '%s'."),
                             project.c_str(), path.c_str(), ambiguous_exception.path1.c_str(), ambiguous_exception.path2.c_str());
    }

    catch( const SearchFilenamesByName::NotFoundException& )
    {
        throw CSProException(_T("The path '%s' does not exist in the project: %s"), path.c_str(), project.c_str());
    }

    catch( const CSProException& exception )
    {
        throw CSProException(_T("There was an error processing the project: %s: %s"), project.c_str(), exception.GetErrorMessage().c_str());
    }
}


std::wstring CSDocCompilerSettings::EvaluateImagePath(const std::wstring& path)
{
    std::wstring evaluated_path = EvaluatePath(path);

    if( PortableFunctions::FileIsRegular(evaluated_path) )
        return evaluated_path;

    // if the file does not exist, look in any specified image directories
    const std::wstring* image_with_name_path = m_docSetSpec->GetSettings().FindInImageDirectories(path);

    return ( image_with_name_path != nullptr ) ? CheckPathCase(*image_with_name_path, path) :
                                                 evaluated_path;
}


std::wstring CSDocCompilerSettings::EvaluateBuildExtra(const std::wstring& path)
{
    std::wstring evaluated_path = EvaluatePath(path);

    if( !PortableFunctions::FileIsRegular(evaluated_path) )
        throw CSProException(_T("The build extra could not be located: ") + evaluated_path);

    return evaluated_path;
}


std::wstring CSDocCompilerSettings::CreateUrlForTitle(const std::wstring& /*path*/)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetTitleLinkageAction() == DocBuildSettings::TitleLinkageAction::Suppress);

    return std::wstring();
}


std::wstring CSDocCompilerSettings::CreateUrlForTopic(const std::wstring& /*project*/, const std::wstring& /*path*/)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || ( GetBuildSettingsDebug()->GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Suppress &&
                                                   GetBuildSettingsDebug()->GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Suppress &&
                                                   GetBuildSettingsDebug()->GetExternalLinkageAction() == DocBuildSettings::ExternalLinkageAction::Suppress) );

    return std::wstring();
}


std::wstring CSDocCompilerSettings::CreateUrlForLogicTopic(const TCHAR* help_topic_filename)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetLogicLinkageAction() == DocBuildSettings::LogicLinkageAction::CSProUsers);

    return CreateUrlForLogicTopicOnCSProUsersForum(help_topic_filename);
}


std::wstring CSDocCompilerSettings::CreateUrlForLogicTopicOnCSProUsersForum(const TCHAR* help_topic_filename)
{
    ASSERT(PortableFunctions::PathGetFileExtension(help_topic_filename) == FileExtensions::HTML);

    return _T("https://www.csprousers.org/help/CSPro/") + Encoders::ToUri(help_topic_filename);
}


std::wstring CSDocCompilerSettings::CreateUrlForLogicHelpTopicInCSProProject(const TCHAR* help_topic_filename)
{
    const std::wstring csdoc_filename = PortableFunctions::PathRemoveFileExtension(help_topic_filename) + FileExtensions::WithDot::CSDocument;
    const std::wstring project = _T("CSPro");
    const std::wstring path = EvaluateTopicPath(project, csdoc_filename);

    return CreateUrlForTopic(project, path);
}


std::wstring CSDocCompilerSettings::CreateUrlForImageFile(const std::wstring& path)
{
    ASSERT(GetBuildSettingsDebug() == nullptr || GetBuildSettingsDebug()->GetImageAction() == DocBuildSettings::ImageAction::DataUrl);

    return Encoders::ToDataUrl(*FileIO::Read(path),
                               ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path))));
}


std::optional<unsigned> CSDocCompilerSettings::GetContextId(const std::wstring& context, bool /*use_if_exists*/)
{
    const std::map<std::wstring, unsigned>& context_ids = m_docSetSpec->GetContextIds();
    const auto& lookup = context_ids.find(context);

    if( lookup != context_ids.cend() )
        return lookup->second;

    return std::nullopt;
}
