#include "StdAfx.h"
#include "DocSetBuilderCache.h"
#include "CSDocCompilerSettings.h"
#include "TitleManager.h"


bool DocSetBuilderCache::LogWrittenFile(const std::wstring& filename, wstring_view text_content_sv)
{
    auto lookup = m_writtenFilesAndLoadedContent.find(filename);

    // if this is the first writing, simply log the filename
    if( lookup == m_writtenFilesAndLoadedContent.cend() )
    {
        m_writtenFilesAndLoadedContent.emplace(filename, nullptr);
        return true;
    }

    // otherwise load the content (if not already done) to make sure it is not different
    if( lookup->second == nullptr )
        lookup->second = std::make_unique<std::wstring>(FileIO::ReadText(filename));

    if( *lookup->second != text_content_sv )
        throw CSProException(_T("Multiple files with different content cannot be written to: ") + filename);

    return false;
}


void DocSetBuilderCache::LogCopiedFile(const std::wstring& filename, const std::tuple<int64_t, time_t>& file_size_and_modified_time)
{
    const auto& lookup = m_copiedFileSizesAndModifiedTimes.find(filename);

    if( lookup == m_copiedFileSizesAndModifiedTimes.cend() )
    {
        m_copiedFileSizesAndModifiedTimes.try_emplace(filename, file_size_and_modified_time);
    }

    // issue an error if multiple files are copied with the same name but with different content
    else if( lookup->second != file_size_and_modified_time )
    {
        throw CSProException(_T("Multiple files with different content cannot be copied to: ") + filename);
    }
}


const std::wstring& DocSetBuilderCache::GetTitle(TitleManager& title_manager, const std::wstring& csdoc_filename)
{
    const std::wstring current_title = title_manager.GetTitle(csdoc_filename);

    auto lookup = m_previouslyRetrievedTitles.find(csdoc_filename);

    if( lookup == m_previouslyRetrievedTitles.cend() )
    {
        lookup = m_previouslyRetrievedTitles.try_emplace(csdoc_filename, current_title).first;
    }

    else if( lookup->second != current_title )
    {
        IssueTitleChangeException(csdoc_filename, current_title);
    }

    return lookup->second;
}


void DocSetBuilderCache::SetTitle(TitleManager& title_manager, const std::wstring& csdoc_filename, const std::wstring& title)
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_filename);

    if( lookup != m_previouslyRetrievedTitles.cend() && title != lookup->second )
        IssueTitleChangeException(csdoc_filename, title);

    title_manager.SetTitle(csdoc_filename, title);
}


void DocSetBuilderCache::ClearTitle(TitleManager& title_manager, const std::wstring& csdoc_filename)
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_filename);

    if( lookup != m_previouslyRetrievedTitles.cend() )
        IssueTitleChangeException(csdoc_filename, _T("<no title>"));

    title_manager.ClearTitle(csdoc_filename);
}


void DocSetBuilderCache::IssueTitleChangeException(const std::wstring& csdoc_filename, const std::wstring& new_title) const
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_filename);
    ASSERT(lookup != m_previouslyRetrievedTitles.cend());

    throw CSProException(_T("You must rebuild the project because the title of '%s' changed after being previously used: '%s' -> '%s'"),
                         PortableFunctions::PathGetFilename(csdoc_filename), lookup->second.c_str(), new_title.c_str());
}


const std::vector<std::wstring>& DocSetBuilderCache::GetStylesheetImagePaths()
{
    if( m_stylesheetImagePaths.empty() )
    {
        DirectoryLister directory_lister;
        directory_lister.SetNameFilter(_T("toc-*.png"));
        m_stylesheetImagePaths = directory_lister.GetPaths(Html::GetDirectory(Html::Subdirectory::Document));
        ASSERT(!m_stylesheetImagePaths.empty());
    }

    return m_stylesheetImagePaths;
}


const std::wstring& DocSetBuilderCache::GetDefaultDocumentPath(const DocSetSpec& doc_set_spec, bool path_must_be_for_default_document)
{
    auto lookup = m_defaultDocumentPaths.find(doc_set_spec.GetFilename());

    auto process_cache = [&]() -> const std::wstring&
    {
        ASSERT(lookup != m_defaultDocumentPaths.cend());

        if( ( std::get<0>(lookup->second).empty() ) ||
            ( path_must_be_for_default_document && !std::get<1>(lookup->second) ) )
        {
            throw CSProException(_T("The Document Set '%s' does not define a default document."), doc_set_spec.GetFilename().c_str());
        }

        return std::get<0>(lookup->second);
    };

    if( lookup != m_defaultDocumentPaths.cend() )
        return process_cache();

    // if not cached, try to identify the default document using a few routines...
    auto cache_and_process = [&](std::wstring path, bool path_is_for_default_document) -> const std::wstring&
    {
        lookup = m_defaultDocumentPaths.try_emplace(doc_set_spec.GetFilename(), std::make_tuple(std::move(path), path_is_for_default_document)).first;
        return process_cache();
    };

    // 1) if a default document is defined, use it
    {
        const DocSetComponent* default_document = doc_set_spec.GetDefaultDocument().get();

        if( default_document != nullptr )
            return cache_and_process(default_document->filename, true);
    }

    // 2) if a table of contents exists, use the first document listed
    if( doc_set_spec.GetTableOfContents().has_value() )
    {
        const DocSetComponent* default_document = doc_set_spec.GetTableOfContents()->GetDocumentByPosition(0);

        if( default_document != nullptr )
            return cache_and_process(default_document->filename, true);
    }

    // 3) if all documents are in the same directory, that directory can be treated as the location of the default document
    {
        const DocSetComponent* first_document = nullptr;
        std::wstring first_document_directory;
        size_t num_csdocs = 0;

        for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
        {
            if( doc_set_component.type != DocSetComponent::Type::Document )
                continue;

            std::wstring document_directory = PortableFunctions::PathGetDirectory(doc_set_component.filename);

            if( ++num_csdocs == 1 )
            {
                first_document = &doc_set_component;
                first_document_directory = std::move(document_directory);
            }

            else if( !SO::EqualsNoCase(document_directory, first_document_directory) )
            {
                first_document = nullptr;
                break;
            }
        }

        if( first_document != nullptr )
        {
            // if only one document exists, it can be considered the default document
            const bool path_is_for_default_document = ( num_csdocs == 1 );
            return cache_and_process(first_document->filename, path_is_for_default_document);
        }
    }

    // there is no default document
    return cache_and_process(std::wstring(), false);
}


const CSDocCompilerSettingsForBuilding& DocSetBuilderCache::GetDocSetForProjectCompiledForDataForTree(const std::wstring& project_doc_set_spec_filename,
                                                                                                      const CSDocCompilerSettingsForBuilding& current_settings)
{
    DocBuildSettings::BuildType current_build_type = current_settings.GetBuildSettings().GetBuildType().value_or(DocBuildSettings::BuildType::HtmlPages);
    DocSetProjectCacheKey cache_key(project_doc_set_spec_filename, current_build_type, current_settings.GetBuildName());

    const auto& lookup = m_docSetProjects.find(cache_key);

    if( lookup != m_docSetProjects.cend() )
        return *lookup->second;

    // compile the Document Set
    auto project_doc_set_spec = std::make_unique<DocSetSpec>(project_doc_set_spec_filename);

    try
    {
        DocSetCompiler doc_set_compiler(DocSetCompiler::ThrowErrors { });
        doc_set_compiler.CompileSpec(*project_doc_set_spec, FileIO::ReadText(project_doc_set_spec_filename), DocSetCompiler::SpecCompilationType::DataForTree);
    }

    catch( const CSProException& exception )
    {
        throw CSProException(_T("The reference to the Document Set '%s' could not be evaluated due to compilation errors: %s"),
                             PortableFunctions::PathGetFilename(project_doc_set_spec_filename), exception.GetErrorMessage().c_str());
    }

    // look in the Document Set for build settings that most match the current build settings
    auto [project_build_settings, project_build_name] = project_doc_set_spec->GetSettings().GetEvaluatedBuildSettings(current_build_type, current_settings.GetBuildName());

    // if the build settings do not define an output directory, set it to the same one as the current compilation
    if( project_build_settings.GetOutputDirectory().empty() )
        project_build_settings.SetOutputDirectory(current_settings.GetOutputDirectoryForRelativeEvaluation(false));

    auto project_settings = CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(project_doc_set_spec), project_build_settings,
                                                                                   current_build_type, std::move(project_build_name), false);

    return *m_docSetProjects.try_emplace(std::move(cache_key), std::move(project_settings)).first->second;
}
