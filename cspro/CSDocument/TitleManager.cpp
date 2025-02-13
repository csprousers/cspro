#include "StdAfx.h"
#include "TitleManager.h"
#include "CSDocCompiler.h"


namespace
{
    // titles will be be persisted for four weeks
    constexpr const TCHAR* CSDocTitlesTableName    = _T("csdoc_titles");
    constexpr int64_t CSDocTitlesExpirationSeconds = DateHelper::SecondsInWeek(4);

    constexpr TCHAR TimeAndTitleSeparator = ';';
}


TitleManager::TitleManager(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   m_settingsDb(CSProExecutables::Program::CSDocument, CSDocTitlesTableName, CSDocTitlesExpirationSeconds),
        m_docSetSpec(std::move(doc_set_spec))
{
}


std::wstring TitleManager::GetTitle(const std::wstring& csdoc_filename)
{
    ASSERT(PortableFunctions::FileIsRegular(csdoc_filename));

    std::wstring title;

    // return a title when a cached title exists and the file has not been modified from when the cached title was set
    if( GetTitleFromCache(title, csdoc_filename) )
        return title;

    // otherwise compile the document for the title
    constexpr const TCHAR* RecursivePreventionMessageText = _T("cannot be accessed before it is set.");

    static std::vector<std::wstring> csdoc_filenames_currently_compiling;

    if( std::find_if(csdoc_filenames_currently_compiling.cbegin(), csdoc_filenames_currently_compiling.cend(),
                     [&](const std::wstring& filename) { return SO::EqualsNoCase(filename, csdoc_filename); }) != csdoc_filenames_currently_compiling.cend() )
    {
        throw CSProException(_T("The title for '%s' %s"), + PortableFunctions::PathGetFilename(csdoc_filename), RecursivePreventionMessageText);
    }

    RAII::PushOnVectorAndPopOnDestruction<std::wstring> filename_holder(csdoc_filenames_currently_compiling, csdoc_filename);

    try
    {
        CSDocCompilerSettingsForTitleManager settings(m_docSetSpec);

        CSDocCompiler csdoc_compiler;
        csdoc_compiler.CompileToHtml(settings, csdoc_filename, FileIO::ReadText(csdoc_filename));
    }

    catch( const CSProException& exception )
    {
        if( exception.GetErrorMessage().find(RecursivePreventionMessageText) != std::wstring::npos )
            throw;
    }

    if( GetTitleFromCache(title, csdoc_filename) )
        return title;

    throw CSProException(_T("The document title is not known for: ") + csdoc_filename);
}


bool TitleManager::GetTitleFromCache(std::wstring& title, const std::wstring& csdoc_filename)
{
    // if a cached title exists, check if the file has been modified from when the cached title was set
    const std::wstring* cached_title = m_settingsDb.Read<std::wstring*>(csdoc_filename);

    if( cached_title != nullptr )
    {
        const size_t semicolon_pos = cached_title->find(TimeAndTitleSeparator);

        if( semicolon_pos != std::wstring::npos &&
            PortableFunctions::FileModifiedTime(csdoc_filename) == CIMSAString::Val(cached_title->substr(0, semicolon_pos)) )
        {
            title = cached_title->substr(semicolon_pos + 1);
            return true;
        }
    }

    return false;
}


void TitleManager::SetTitle(const std::wstring& csdoc_filename, const std::wstring* title)
{
    if( csdoc_filename.empty() )
        return;

    ASSERT(PortableFunctions::FileIsRegular(csdoc_filename));
    const int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(csdoc_filename);

    std::wstring cached_title = CS2WS(IntToString(file_on_disk_modified_time));

    if( title != nullptr )
    {
        cached_title.push_back(TimeAndTitleSeparator);
        cached_title.append(*title);
    }

    m_settingsDb.Write(csdoc_filename, cached_title);
}
