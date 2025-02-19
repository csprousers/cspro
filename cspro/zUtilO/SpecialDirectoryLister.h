#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/MediaStore.h>
#include <zToolsO/DirectoryLister.h>

class VirtualDirectoryLister;
namespace Nodes::Path { enum class FilterType : int; }


class CLASS_DECL_ZUTILO SpecialDirectoryLister : public DirectoryLister
{
public:
    static constexpr char SpecialFilterPrefix                        = '|';

    static constexpr char  SpecialDirectoryPrefix                    = '|';
    static constexpr std::string_view SpecialDirectoryAndroidRoot_sv = "|Android";

    // returns the filter for a given filter type
    static std::string EvaluateFilter(Nodes::Path::FilterType filter_type);

    // evaluates a filter, which, if preceeded by SpecialFilterPrefix, will by assumed to be a filter type;
    // multiple filters can be passed to the function; e.g.,: *.jpg;|FileType.Audio
    // an exception is thrown if the filter type is not valid
    static std::string EvaluateFilter(std::string_view filter_sv);

    // returns whether or not a directory might be a special directory
    static bool IsSpecialDirectory(std::string_view directory_sv)
    {
        return ( !directory_sv.empty() && directory_sv.front() == SpecialDirectoryPrefix );
    }

    // evaluates a directory, which, if preceeded by SpecialDirectoryPrefix, will be assumed to be a special directory;
    // the special directories only exist on Android:
    // - Android root
    // - a MediaStore directory
    // an exception is thrown if the special directory cannot be parsed
    struct AndroidRoot { };
    struct MediaStoreDirectory { MediaStore::MediaType media_type; std::string subdirectory; };
    using SpecialDirectory = std::variant<std::string, AndroidRoot, MediaStoreDirectory>;

    static SpecialDirectory EvaluateSpecialDirectory(std::string_view directory_sv);

    // returns whether or a not a special directory exists
    static bool SpecialDirectoryExists(const SpecialDirectory& special_directory, bool for_disk_paths_only_return_true_for_directories);

    // throws an exception if the start directory is not within the root directory
    static void ValidateStartAndRootSpecialDirectories(const SpecialDirectory& start_directory, const SpecialDirectory& root_directory);

    // returns the name of a special directory
    static std::string GetSpecialDirectoryName(const SpecialDirectory& special_directory);

    // returns the path of a special directory (with no trailing slash)
    static std::string GetSpecialDirectoryPath(const SpecialDirectory& special_directory);

protected:
    static std::string CreateMediaStoreDirectoryPath(MediaStore::MediaType media_type, const std::string& subdirectory);

    static std::shared_ptr<const VirtualDirectoryLister> GetVirtualDirectoryLister(MediaStore::MediaType media_type);


    // SpecialDirectoryLister derives from DirectoryLister so that DirectoryLister's flags are available
private:
    using DirectoryLister::DirectoryLister;

public:
    virtual ~SpecialDirectoryLister() { }

    // the creation routines throws an exception is the directory is not valid
    static std::unique_ptr<SpecialDirectoryLister> CreateSpecialDirectoryLister(const SpecialDirectory& special_directory,
                                                                                bool recursive = false, bool include_files = true, bool include_directories = false,
                                                                                bool include_trailing_slash_on_directories = true, bool filter_directories = true);

    virtual std::vector<std::string> GetSpecialPaths() = 0;
    virtual std::optional<std::string> GetParentDirectory() = 0;
};
