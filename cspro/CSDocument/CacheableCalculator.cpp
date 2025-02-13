#include "StdAfx.h"
#include "CacheableCalculator.h"


namespace
{
    struct Data
    {
        std::map<std::tuple<StringNoCase, bool, StringNoCase>, std::map<StringNoCase, std::vector<std::wstring>>> files_of_type_in_directory;
        std::map<StringNoCase, std::wstring> project_doc_set_spec_filenames;
        std::map<StringNoCase, std::tuple<int64_t, std::vector<std::wstring>>> project_documents;
    };

    Data& GetData()
    {
        static Data data;
        return data;
    }
}


void CacheableCalculator::ResetCache()
{
    Data& data = GetData();
    data.files_of_type_in_directory.clear();
    data.project_doc_set_spec_filenames.clear();
    data.project_documents.clear();
}


const std::wstring* CacheableCalculator::FindFileByNameInDirectory(const std::wstring& directory, bool recursive, const std::wstring& name, const std::wstring* already_matched_path/* = nullptr*/)
{
    // check the cache
    Data& data = GetData();
    const StringNoCase extension = PortableFunctions::PathGetFileExtension(name);
    std::tuple<StringNoCase, bool, StringNoCase> cache_key(directory, recursive, extension);
    const auto& listing_lookup = data.files_of_type_in_directory.find(cache_key);

    if( listing_lookup != data.files_of_type_in_directory.cend() )
    {
        const std::wstring* matched_path = FindFileByNameInDirectory(listing_lookup->second, name, already_matched_path);

        if( matched_path != nullptr )
            return matched_path;

        data.files_of_type_in_directory.erase(listing_lookup);
    }

    // if not in the cache, generate the directory listing by extension and cache it
    DirectoryLister directory_lister(recursive, true, false, true, false);
    directory_lister.SetNameFilter(_T("*.") + extension);

    std::map<StringNoCase, std::vector<std::wstring>> files_in_directory_by_name;

    for( std::wstring& path : directory_lister.GetPaths(directory) )
    {
        StringNoCase filename_only = PortableFunctions::PathGetFilename(path);
        files_in_directory_by_name[std::move(filename_only)].emplace_back(std::move(path));
    }

    const std::map<StringNoCase, std::vector<std::wstring>>& cached_files_in_directory_by_name =
         data.files_of_type_in_directory.try_emplace(std::move(cache_key),
                                                     std::move(files_in_directory_by_name)).first->second;

    return FindFileByNameInDirectory(cached_files_in_directory_by_name, name, already_matched_path);
}


const std::wstring* CacheableCalculator::FindFileByNameInDirectory(const std::map<StringNoCase, std::vector<std::wstring>>& files_in_directory_by_name, const std::wstring& name, const std::wstring* already_matched_path)
{
    const auto& file_lookup = files_in_directory_by_name.find(name);

    if( file_lookup != files_in_directory_by_name.end() )
    {
        for( const std::wstring& filename : file_lookup->second )
        {
            if( PortableFunctions::FileIsRegular(filename) )
            {
                if( already_matched_path != nullptr && !SO::EqualsNoCase(filename, *already_matched_path) )
                {
                    throw CSProException(_T("The name '%s' is ambiguous. It could refer to '%s' or '%s'."),
                                         name.c_str(), already_matched_path->c_str(), filename.c_str());

                }

                already_matched_path = &filename;
            }
        }
    }

    return already_matched_path;
}


std::wstring CacheableCalculator::FindProjectDocSetSpecFilename(const std::wstring& project_directory)
{
    // check the cache
    Data& data = GetData();
    const auto& lookup = data.project_doc_set_spec_filenames.find(project_directory);

    if( lookup != data.project_doc_set_spec_filenames.cend() )
    {
        if( PortableFunctions::FileIsRegular(lookup->second) )
            return lookup->second;

        data.project_doc_set_spec_filenames.erase(lookup);
    }

    // if not in the cache, look in the project directory for all .csdocset files
    DirectoryLister directory_lister(true, true, false, true, false);
    directory_lister.SetNameFilter(FileExtensions::Wildcard::CSDocumentSet);
            
    std::vector<std::wstring> doc_set_spec_filenames = directory_lister.GetPaths(project_directory);

    if( doc_set_spec_filenames.size() == 1 )
    {
        // cache the value and return it
        return data.project_doc_set_spec_filenames.try_emplace(project_directory,
                                                               std::move(doc_set_spec_filenames.front())).first->second;
    }

    else if( doc_set_spec_filenames.empty() )
    {
        throw CSProException(_T("No CSPro Document Sets could be found in the project directory: ") + project_directory);
    }

    else
    {
        throw CSProException(_T("More than one CSPro Document Set cannot be located in the project directory: ") + project_directory);
    }
}


const std::vector<std::wstring>& CacheableCalculator::GetDocumentFilenamesForProject(const std::wstring& project_doc_set_spec_filename)
{
    ASSERT(PortableFunctions::FileIsRegular(project_doc_set_spec_filename));
    const int64_t file_modified_time = PortableFunctions::FileModifiedTime(project_doc_set_spec_filename);

    // check the cache
    Data& data = GetData();
    const auto& lookup = data.project_documents.find(project_doc_set_spec_filename);

    if( lookup != data.project_documents.cend() )
    {
        if( std::get<0>(lookup->second) == file_modified_time )
            return std::get<1>(lookup->second);

        data.project_documents.erase(lookup);
    }

    // if not in the cache, compile the spec to get the document filenames
    DocSetSpec doc_set_spec(project_doc_set_spec_filename);

    DocSetCompiler doc_set_compiler(DocSetCompiler::SuppressErrors { });
    doc_set_compiler.CompileSpec(doc_set_spec, FileIO::ReadText(project_doc_set_spec_filename), DocSetCompiler::SpecCompilationType::DataForTree);

    std::vector<std::wstring> document_filenames;

    for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
            document_filenames.emplace_back(doc_set_component.filename);
    }

    return std::get<1>(data.project_documents.try_emplace(project_doc_set_spec_filename,
                                                          std::make_tuple(file_modified_time, std::move(document_filenames))).first->second);
}
