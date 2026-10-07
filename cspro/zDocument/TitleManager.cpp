#include "StdAfx.h"
#include "TitleManager.h"
#include "CSDocCompiler.h"


namespace
{
    // titles will be be persisted for four weeks
    constexpr const char* CSDocTitlesTableName     = "csdoc_titles";
    constexpr int64_t CSDocTitlesExpirationSeconds = DateHelper::SecondsInWeek(4);

    constexpr char TimeAndTitleSeparator = ';';
}


TitleManager::TitleManager(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   m_settingsDb(CSProExecutables::Program::CSDocument, CSDocTitlesTableName, CSDocTitlesExpirationSeconds),
        m_docSetSpec(std::move(doc_set_spec))
{
}


std::string TitleManager::GetTitle(const std::string& csdoc_file_path)
{
    ASSERT(PortableFunctions::FileIsRegular(csdoc_file_path));

    std::string title;

    // return a title when a cached title exists and the file has not been modified from when the cached title was set
    if( GetTitleFromCache(title, csdoc_file_path) )
        return title;

    // otherwise compile the document for the title
    constexpr const char* RecursivePreventionMessageText = "cannot be accessed before it is set.";

    static std::vector<std::string> csdoc_file_paths_currently_compiling;

    if( std::find_if(csdoc_file_paths_currently_compiling.cbegin(), csdoc_file_paths_currently_compiling.cend(),
                     [&](const std::string& file_path) { return SO::EqualsNoCase(file_path, csdoc_file_path); }) != csdoc_file_paths_currently_compiling.cend() )
    {
        throw CSProException("The title for '%s' %s", + PortableFunctions::PathGetFilename(csdoc_file_path).c_str(), RecursivePreventionMessageText);
    }

    const RAII::PushOnVectorAndPopOnDestruction<std::string> file_path_holder(csdoc_file_paths_currently_compiling, csdoc_file_path);

    try
    {
        CSDocCompilerSettingsForTitleManager settings(m_docSetSpec);

        CSDocCompiler csdoc_compiler;
        csdoc_compiler.CompileToHtml(settings, csdoc_file_path, FileIO::ReadText(csdoc_file_path));
    }

    catch( const CSProException& exception )
    {
        if( std::string_view(exception.what()).find(RecursivePreventionMessageText) != std::string_view::npos )
            throw;
    }

    if( GetTitleFromCache(title, csdoc_file_path) )
        return title;

    throw CSProException("The document title is not known for: %s", csdoc_file_path.c_str());
}


bool TitleManager::GetTitleFromCache(std::string& title, const std::string& csdoc_file_path)
{
    // if a cached title exists, check if the file has been modified from when the cached title was set
    const std::string* const cached_title = m_settingsDb.Read<std::string*>(csdoc_file_path);

    if( cached_title != nullptr )
    {
        const size_t semicolon_pos = cached_title->find(TimeAndTitleSeparator);

        if( semicolon_pos != std::string::npos &&
            PortableFunctions::FileModifiedTime(csdoc_file_path) == CIMSAString::Val(cached_title->substr(0, semicolon_pos)) )
        {
            title = cached_title->substr(semicolon_pos + 1);
            return true;
        }
    }

    return false;
}


void TitleManager::SetTitle(const std::string& csdoc_file_path, const std::string* const title)
{
    if( csdoc_file_path.empty() )
        return;

    ASSERT(PortableFunctions::FileIsRegular(csdoc_file_path));
    const int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(csdoc_file_path);

    std::string cached_title = IntToString(file_on_disk_modified_time);

    if( title != nullptr )
    {
        cached_title.push_back(TimeAndTitleSeparator);
        cached_title.append(*title);
    }

    m_settingsDb.Write(csdoc_file_path, cached_title);
}
