#include "StdAfx.h"
#include "CacheableCalculator.h"


namespace
{
    struct Data
    {
        std::map<std::tuple<std::string, bool, std::string>, std::map<std::string, std::vector<std::string>, cs::case_insensitive_less>> files_of_type_in_directory;
        std::map<std::string, std::string, cs::case_insensitive_less> project_doc_set_spec_file_paths;
        std::map<std::string, std::tuple<int64_t, std::vector<std::string>>, cs::case_insensitive_less> project_documents;
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
    data.project_doc_set_spec_file_paths.clear();
    data.project_documents.clear();
}


const std::string* CacheableCalculator::FindFileByNameInDirectory(const std::string& directory, const bool recursive, const std::string& filename,
                                                                  const std::string* const already_matched_file_path/* = nullptr*/)
{
    // check the cache
    Data& data = GetData();
    const std::string extension = SO::ToLower(PortableFunctions::PathGetFileExtension(filename));
    std::tuple<std::string, bool, std::string> cache_key(SO::ToLower(directory), recursive, extension);
    const auto& listing_lookup = data.files_of_type_in_directory.find(cache_key);

    if( listing_lookup != data.files_of_type_in_directory.cend() )
    {
        const std::string* const matched_path = FindFileByNameInDirectory(listing_lookup->second, filename, already_matched_file_path);

        if( matched_path != nullptr )
            return matched_path;

        data.files_of_type_in_directory.erase(listing_lookup);
    }

    // if not in the cache, generate the directory listing by extension and cache it
    DirectoryLister directory_lister(recursive, true, false, true, false);
    directory_lister.SetNameFilter(FileExtensions::CreateWildcard(extension));

    std::map<std::string, std::vector<std::string>, cs::case_insensitive_less> file_paths_in_directory_by_name;

    for( std::string& path : directory_lister.GetPaths(directory) )
    {
        std::string filename_only = PortableFunctions::PathGetFilename(path);
        file_paths_in_directory_by_name[std::move(filename_only)].emplace_back(std::move(path));
    }

    const std::map<std::string, std::vector<std::string>, cs::case_insensitive_less>& cached_files_in_directory_by_name =
         data.files_of_type_in_directory.try_emplace(std::move(cache_key),
                                                     std::move(file_paths_in_directory_by_name)).first->second;

    return FindFileByNameInDirectory(cached_files_in_directory_by_name, filename, already_matched_file_path);
}


const std::string* CacheableCalculator::FindFileByNameInDirectory(const std::map<std::string, std::vector<std::string>, cs::case_insensitive_less>& file_paths_in_directory_by_name,
                                                                  const std::string& filename, const std::string* already_matched_file_path)
{
    const auto& file_lookup = file_paths_in_directory_by_name.find(filename);

    if( file_lookup != file_paths_in_directory_by_name.end() )
    {
        for( const std::string& file_path : file_lookup->second )
        {
            if( PortableFunctions::FileIsRegular(file_path) )
            {
                if( already_matched_file_path != nullptr && !SO::EqualsNoCase(file_path, *already_matched_file_path) )
                {
                    throw CSProException("The name '%s' is ambiguous. It could refer to '%s' or '%s'.",
                                         filename.c_str(), already_matched_file_path->c_str(), file_path.c_str());

                }

                already_matched_file_path = &file_path;
            }
        }
    }

    return already_matched_file_path;
}


std::string CacheableCalculator::FindProjectDocSetSpecFilePath(const std::string& project_directory)
{
    // check the cache
    Data& data = GetData();
    const auto& lookup = data.project_doc_set_spec_file_paths.find(project_directory);

    if( lookup != data.project_doc_set_spec_file_paths.cend() )
    {
        if( PortableFunctions::FileIsRegular(lookup->second) )
            return lookup->second;

        data.project_doc_set_spec_file_paths.erase(lookup);
    }

    // if not in the cache, look in the project directory for all .csdocset files
    DirectoryLister directory_lister(true, true, false, true, false);
    directory_lister.SetNameFilter(FileExtensions::CreateWildcard(FileExtensions::CSDocumentSet));

    std::vector<std::string> doc_set_spec_file_paths = directory_lister.GetPaths(project_directory);

    if( doc_set_spec_file_paths.size() == 1 )
    {
        // cache the value and return it
        return data.project_doc_set_spec_file_paths.try_emplace(project_directory,
                                                                std::move(doc_set_spec_file_paths.front())).first->second;
    }

    else if( doc_set_spec_file_paths.empty() )
    {
        throw CSProException("No CSPro Document Sets could be found in the project directory: %s",
                             project_directory.c_str());
    }

    else
    {
        throw CSProException("More than one CSPro Document Set cannot be located in the project directory: %s",
                             project_directory.c_str());
    }
}


const std::vector<std::string>& CacheableCalculator::GetDocumentFilePathsForProject(const std::string& project_doc_set_spec_file_path)
{
    ASSERT(PortableFunctions::FileIsRegular(project_doc_set_spec_file_path));
    const int64_t file_modified_time = PortableFunctions::FileModifiedTime(project_doc_set_spec_file_path);

    // check the cache
    Data& data = GetData();
    const auto& lookup = data.project_documents.find(project_doc_set_spec_file_path);

    if( lookup != data.project_documents.cend() )
    {
        if( std::get<0>(lookup->second) == file_modified_time )
            return std::get<1>(lookup->second);

        data.project_documents.erase(lookup);
    }

    // if not in the cache, compile the spec to get the document filenames
    DocSetSpec doc_set_spec(project_doc_set_spec_file_path);

    DocSetCompiler doc_set_compiler(DocSetCompiler::SuppressErrors { });
    doc_set_compiler.CompileSpec(doc_set_spec, FileIO::ReadText(project_doc_set_spec_file_path), DocSetCompiler::SpecCompilationType::DataForTree);

    std::vector<std::string> document_file_paths;

    for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
            document_file_paths.emplace_back(doc_set_component.file_path);
    }

    return std::get<1>(data.project_documents.try_emplace(project_doc_set_spec_file_path,
                                                          std::make_tuple(file_modified_time, std::move(document_file_paths))).first->second);
}
