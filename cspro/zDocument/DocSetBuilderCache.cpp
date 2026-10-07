#include "StdAfx.h"
#include "DocSetBuilderCache.h"
#include "CSDocCompilerSettings.h"
#include "TitleManager.h"


bool DocSetBuilderCache::LogWrittenFile(const std::string& file_path, const std::string_view text_content_sv)
{
    auto lookup = m_writtenFilesAndLoadedContent.find(file_path);

    // if this is the first writing, simply log the filename
    if( lookup == m_writtenFilesAndLoadedContent.cend() )
    {
        m_writtenFilesAndLoadedContent.emplace(file_path, nullptr);
        return true;
    }

    // otherwise load the content (if not already done) to make sure it is not different
    if( lookup->second == nullptr )
        lookup->second = std::make_unique<std::string>(FileIO::ReadText(file_path));

    if( *lookup->second != text_content_sv )
        throw CSProException("Multiple files with different content cannot be written to: %s", file_path.c_str());

    return false;
}


void DocSetBuilderCache::LogCopiedFile(const std::string& file_path, const std::tuple<int64_t, int64_t>& file_size_and_modified_time)
{
    const auto& lookup = m_copiedFileSizesAndModifiedTimes.find(file_path);

    if( lookup == m_copiedFileSizesAndModifiedTimes.cend() )
    {
        m_copiedFileSizesAndModifiedTimes.try_emplace(file_path, file_size_and_modified_time);
    }

    // issue an error if multiple files are copied with the same name but with different content
    else if( lookup->second != file_size_and_modified_time )
    {
        throw CSProException("Multiple files with different content cannot be copied to: %s", file_path.c_str());
    }
}


const std::string& DocSetBuilderCache::GetTitle(TitleManager& title_manager, const std::string& csdoc_file_path)
{
    std::string current_title = title_manager.GetTitle(csdoc_file_path);

    auto lookup = m_previouslyRetrievedTitles.find(csdoc_file_path);

    if( lookup == m_previouslyRetrievedTitles.cend() )
    {
        lookup = m_previouslyRetrievedTitles.try_emplace(csdoc_file_path, std::move(current_title)).first;
    }

    else if( lookup->second != current_title )
    {
        IssueTitleChangeException(csdoc_file_path, current_title);
    }

    return lookup->second;
}


void DocSetBuilderCache::SetTitle(TitleManager& title_manager, const std::string& csdoc_file_path, const std::string& title)
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_file_path);

    if( lookup != m_previouslyRetrievedTitles.cend() && title != lookup->second )
        IssueTitleChangeException(csdoc_file_path, title);

    title_manager.SetTitle(csdoc_file_path, title);
}


void DocSetBuilderCache::ClearTitle(TitleManager& title_manager, const std::string& csdoc_file_path)
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_file_path);

    if( lookup != m_previouslyRetrievedTitles.cend() )
        IssueTitleChangeException(csdoc_file_path, "<no title>");

    title_manager.ClearTitle(csdoc_file_path);
}


void DocSetBuilderCache::IssueTitleChangeException(const std::string& csdoc_file_path, const cs::string_sz new_title) const
{
    const auto& lookup = m_previouslyRetrievedTitles.find(csdoc_file_path);
    ASSERT(lookup != m_previouslyRetrievedTitles.cend());

    throw CSProException("You must rebuild the project because the title of '%s' changed after being previously used: '%s' -> '%s'",
                         PortableFunctions::PathGetFilename(csdoc_file_path).c_str(),
                         lookup->second.c_str(),
                         new_title.c_str());
}


const std::vector<std::string>& DocSetBuilderCache::GetStylesheetImageFilePaths()
{
    if( m_stylesheetImageFilePaths.empty() )
    {
        DirectoryLister directory_lister;
        directory_lister.SetNameFilter("toc-*.png");
        m_stylesheetImageFilePaths = directory_lister.GetPaths(Html::GetDirectory(Html::Subdirectory::Document));
        ASSERT(!m_stylesheetImageFilePaths.empty());
    }

    return m_stylesheetImageFilePaths;
}


const std::string& DocSetBuilderCache::GetDefaultDocumentFilePath(const DocSetSpec& doc_set_spec, const bool path_must_be_for_default_document)
{
    auto lookup = m_defaultDocumentFilePaths.find(doc_set_spec.GetFilePath());

    auto process_cache = [&]() -> const std::string&
    {
        ASSERT(lookup != m_defaultDocumentFilePaths.cend());

        if( ( std::get<0>(lookup->second).empty() ) ||
            ( path_must_be_for_default_document && !std::get<1>(lookup->second) ) )
        {
            throw CSProException("The Document Set '%s' does not define a default document.",
                                 doc_set_spec.GetFilePath().c_str());
        }

        return std::get<0>(lookup->second);
    };

    if( lookup != m_defaultDocumentFilePaths.cend() )
        return process_cache();

    // if not cached, try to identify the default document using a few routines...
    auto cache_and_process = [&](std::string path, const bool path_is_for_default_document) -> const std::string&
    {
        lookup = m_defaultDocumentFilePaths.try_emplace(doc_set_spec.GetFilePath(), std::make_tuple(std::move(path), path_is_for_default_document)).first;
        return process_cache();
    };

    // 1) if a default document is defined, use it
    {
        const DocSetComponent* const default_document = doc_set_spec.GetDefaultDocument().get();

        if( default_document != nullptr )
            return cache_and_process(default_document->file_path, true);
    }

    // 2) if a table of contents exists, use the first document listed
    if( doc_set_spec.GetTableOfContents().has_value() )
    {
        const DocSetComponent* const default_document = doc_set_spec.GetTableOfContents()->GetDocumentByPosition(0);

        if( default_document != nullptr )
            return cache_and_process(default_document->file_path, true);
    }

    // 3) if all documents are in the same directory, that directory can be treated as the location of the default document
    {
        const DocSetComponent* first_document = nullptr;
        std::string first_document_directory;
        size_t num_csdocs = 0;

        for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
        {
            if( doc_set_component.type != DocSetComponent::Type::Document )
                continue;

            std::string document_directory = PortableFunctions::PathGetDirectory(doc_set_component.file_path);

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
            return cache_and_process(first_document->file_path, path_is_for_default_document);
        }
    }

    // there is no default document
    return cache_and_process(std::string(), false);
}


const CSDocCompilerSettingsForBuilding& DocSetBuilderCache::GetDocSetForProjectCompiledForDataForTree(const std::string& project_doc_set_spec_file_path,
                                                                                                      const CSDocCompilerSettingsForBuilding& current_settings)
{
    DocBuildSettings::BuildType current_build_type = current_settings.GetBuildSettings().GetBuildType().value_or(DocBuildSettings::BuildType::HtmlPages);
    DocSetProjectCacheKey cache_key(SO::ToLower(project_doc_set_spec_file_path), current_build_type, current_settings.GetBuildName());

    const auto& lookup = m_docSetProjects.find(cache_key);

    if( lookup != m_docSetProjects.cend() )
        return *lookup->second;

    // compile the Document Set
    auto project_doc_set_spec = std::make_unique<DocSetSpec>(project_doc_set_spec_file_path);

    try
    {
        DocSetCompiler doc_set_compiler(DocSetCompiler::ThrowErrors { });
        doc_set_compiler.CompileSpec(*project_doc_set_spec, FileIO::ReadText(project_doc_set_spec_file_path), DocSetCompiler::SpecCompilationType::DataForTree);
    }

    catch( const CSProException& exception )
    {
        throw CSProException("The reference to the Document Set '%s' could not be evaluated due to compilation errors: %s",
                             PortableFunctions::PathGetFilename(project_doc_set_spec_file_path).c_str(),
                             exception.what());
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
