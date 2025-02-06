#include "StdAfx.h"
#include "Creator.h"
#include "GitIgnoreEvaluator.h"
#include <zToolsO/DirectoryLister.h>
#include <zJson/JsonSpecFile.h>
#include <zUtilO/CSProExecutables.h>
#include <zNetwork/CurlHttpConnection.h>


struct Creator::Data
{
    std::string overrides_directory;
    git_repository* repo;
    LoggingListBox* logging_list_box = nullptr;
    const std::string* output_directory = nullptr;
    std::vector<std::string> repo_paths;
    std::map<std::string, std::string> repo_blob_ids;
};


Creator::Creator()
    :   m_data(std::make_unique<Data>())
{
    m_data->overrides_directory = MakeFullPath(CSProExecutables::GetApplicationDirectory(), "..\\Overrides");

    const std::string git_directory = MakeFullPath(CSProExecutables::GetApplicationDirectory(), "..\\..\\..\\.git");

    if( git_repository_open_bare(&m_data->repo, git_directory.c_str()) < 0 )
        ThrowGitException();
}


Creator::~Creator()
{
    git_repository_free(m_data->repo);
}


std::vector<Git::Tag> Creator::GetTags() const
{
    std::vector<Git::Tag> tags;

    struct CB
    {
        static int func(const char* const name, git_oid* const oid, void* const payload)
        {
            auto tags = reinterpret_cast<std::vector<Git::Tag>*>(payload);
            tags->emplace_back(Git::Tag { ObjectIdToString(oid), name });
            return 0;
        };
    };

    git_tag_foreach(m_data->repo, CB::func, &tags);

    return tags;
}


void Creator::CreateRelease(LoggingListBox& logging_list_box, const cs::string_sz commit_string, const std::string& output_directory)
{
    m_data->logging_list_box = &logging_list_box;
    m_data->output_directory = &output_directory;
    m_data->repo_paths.clear();

    m_data->logging_list_box->Clear();
    m_data->logging_list_box->AddText(FormatText("Creating open source release from commit: %s", commit_string.c_str()));

    // ensure that the commit is valid
    git_oid oid;
    git_commit* commit;

    if( git_oid_fromstr(&oid, commit_string.c_str()) < 0 ||
        git_commit_lookup(&commit, m_data->repo, &oid) < 0 )
    {
        throw CSProException("The commit was not found in the repo: %s", commit_string.c_str());
    }

    const git_signature* const author = git_commit_author(commit);
    m_data->logging_list_box->AddText(std::string("    Author: ").append(author->name));
    m_data->logging_list_box->AddText(std::string("    Date: ").append(DateTime::LocalDateTimeString(author->when.time)));
    m_data->logging_list_box->AddText(std::string("    Message: ").append(SO::Trim(std::string_view(git_commit_message(commit)))));

    // get the list of the files that are part of this release
    git_tree* tree;

    if( git_commit_tree(&tree, commit) < 0 )
        ThrowGitException();

    PopulateRepoPaths(tree, "");

    // remove files that should not be part of the open source release
    PruneRepoPaths();

    // remove all existing non-Git files from the output directory...
    PrepareOutputDirectory();

    // ...and then copy the current release files
    CopyFilesToOutputDirectory();

    // copy dummy files for some sensitive files
    CopyReplacementFiles();

    // create a version of SQLite without the SQLite Encryption Extension (SEE)
    CreateSqliteWithoutSEE(tree);

    git_tree_free(tree);

    git_commit_free(commit);
}


template<typename git_oidT>
std::string Creator::ObjectIdToString(const git_oidT* const oid)
{
    constexpr size_t SHA256HexSize = 64;
    char buffer[SHA256HexSize + 1];

    return std::string(git_oid_tostr(buffer, _countof(buffer), oid),
                       SHA256HexSize);
}


template<typename git_treeT>
void Creator::PopulateRepoPaths(const git_treeT* const tree, const std::string& base_path)
{
    const size_t count = git_tree_entrycount(tree);

    for( size_t i = 0; i < count; ++i )
    {
        const git_tree_entry* const tree_entry = git_tree_entry_byindex(tree, i);

        if( tree_entry == nullptr )
            ThrowGitException();

        std::string repo_path = Path::Combine(base_path, git_tree_entry_name(tree_entry));
        ASSERT(repo_path == Path::ToNativeSlash(repo_path));

        git_object* object;

        if( git_tree_entry_to_object(&object, git_tree_owner(tree), tree_entry) < 0 )
            ThrowGitException();

        const git_object_t entry_type = git_tree_entry_type(tree_entry);

        // entries will be another tree...
        if( entry_type == GIT_OBJECT_TREE )
        {
            PopulateRepoPaths(reinterpret_cast<const git_tree*>(object), repo_path);
        }

        // ... or a file
        else if( entry_type == GIT_OBJECT_BLOB )
        {
            m_data->repo_paths.emplace_back(repo_path);
            m_data->repo_blob_ids.try_emplace(std::move(repo_path), ObjectIdToString(git_object_id(object)));
        }

        else
        {
            ASSERT(false);
        }

        git_object_free(object);
    }
}


void Creator::PruneRepoPaths()
{
    const std::string exclusions_file_path = Path::Combine(m_data->overrides_directory, "exclusions.txt");

    m_data->logging_list_box->AddText(SharableString());
    m_data->logging_list_box->AddText(FormatText("Pruning files based on gitignore rules from: %s", exclusions_file_path.c_str()));

    std::vector<std::string>& repo_paths = m_data->repo_paths;
    const size_t initial_file_count = repo_paths.size();

    Git::IgnoreEvaluator gitignore_evaluator;
    gitignore_evaluator.AddRulesFromFile(exclusions_file_path);

    for( size_t i = repo_paths.size() - 1; i < repo_paths.size(); --i )
    {
        if( gitignore_evaluator.Ignore(repo_paths[i]) )
            repo_paths.erase(repo_paths.begin() + i);
    }

    m_data->logging_list_box->AddText(FormatText("Pruned files from %d to %d.", static_cast<int>(initial_file_count),
                                                                                static_cast<int>(repo_paths.size())));
}


void Creator::PrepareOutputDirectory()
{
    // move all non-Git files to a temporary directory, which will then be recycled
    const std::string temp_directory = GetUniqueTempFilePath("CSPro-Open-Source-Old-Files");

    m_data->logging_list_box->AddText(SharableString());
    m_data->logging_list_box->AddText(FormatText("Moving existing open source files to: %s", temp_directory.c_str()));

    FileIO::CreateDirectories(temp_directory);

    SHFILEOPSTRUCT info = { nullptr };
    wchar_t complete_from_path[MAX_PATH];
    wchar_t complete_to_path[MAX_PATH];
    info.wFunc = FO_MOVE;
    info.fFlags = FOF_NOCONFIRMATION;
    info.pFrom = complete_from_path;
    info.pTo = complete_to_path;

    auto to_wide = [&](const std::string& path, wchar_t* const complete_path)
    {
        const int path_length = GetFullPathName(TC::ToWide(path).c_str(), MAX_PATH, complete_path, nullptr);

        if( path_length == 0 || path_length >= MAX_PATH )
            throw CSProException("GetFullPathName error: %s", path.c_str());
    };

    DirectoryLister directory_lister(false, true, true, false);

    for( const std::string& path : directory_lister.GetPaths(*m_data->output_directory) )
    {
        if( Path::GetFilename(path) == ".git" )
            continue;

        const std::string temp_path = Path::Combine(temp_directory, Path::GetFilename(path));

        to_wide(path, complete_from_path);
        to_wide(temp_path, complete_to_path);

        if( SHFileOperation(&info) != 0 )
            throw CSProException("Error moving '%s' to '%s'.", path.c_str(), temp_path.c_str());
    }

    info.wFunc = FO_DELETE;
    info.fFlags |= FOF_ALLOWUNDO;
    to_wide(temp_directory, complete_from_path);
    info.pTo = nullptr;

    m_data->logging_list_box->AddText(FormatText("Recycling: %s", temp_directory.c_str()));

    if( SHFileOperation(&info) != 0 )
        throw CSProException("Error recycling: %s", temp_directory.c_str());
}


void Creator::CopyFilesToOutputDirectory()
{
    m_data->logging_list_box->AddText(SharableString());
    m_data->logging_list_box->AddText(FormatText("Copying %d files to: %s", static_cast<int>(m_data->repo_paths.size()),
                                                                            m_data->output_directory->c_str()));

    git_oid oid;
    git_blob* blob = nullptr;
    uint64_t total_content_size = 0;

    constexpr double PercentReportingInterval = 5;
    const double percent_multiplier = CreatePercentMultiplier(m_data->repo_paths.size());
    double percent = 0;
    double next_percent_for_reporting = PercentReportingInterval;

    for( const std::string& repo_path : m_data->repo_paths )
    {
        if( git_oid_fromstr(&oid, m_data->repo_blob_ids.find(repo_path)->second.c_str()) < 0 ||
            git_blob_lookup(&blob, m_data->repo, &oid) < 0 )
        {
            ThrowGitException();
        }

        const size_t content_size = static_cast<size_t>(git_blob_rawsize(blob));
        total_content_size += content_size;

        const std::byte* const content = static_cast<const std::byte*>(git_blob_rawcontent(blob));

        if( content == nullptr )
            ThrowGitException();

        const std::string output_file_path = Path::Combine(*m_data->output_directory, repo_path);
        FileIO::CreateDirectoriesForFile(output_file_path);

        FileIO::Write(output_file_path, content, content_size);

        git_blob_free(blob);

        percent += percent_multiplier;

        if( percent >= next_percent_for_reporting )
        {
            m_data->logging_list_box->AddText(FormatText("Copy percent: %d", static_cast<int>(100 * percent)));
            next_percent_for_reporting += PercentReportingInterval;
        }
    }

    m_data->logging_list_box->AddText(FormatText("Copied bytes: " Formatter_uint64_t, total_content_size));
}


void Creator::CopyReplacementFiles()
{
    const std::string replacements_file_path = Path::Combine(m_data->overrides_directory, "replacements.json");

    m_data->logging_list_box->AddText(SharableString());
    m_data->logging_list_box->AddText(FormatText("Copying replacement files specified in: %s", replacements_file_path.c_str()));

    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

    for( const JsonNode& replacement_json_node : json_reader->GetArray() )
    {
        const std::string repo_path = Path::ToNativeSlash(replacement_json_node.Get<std::string>("repoPath"));
        const std::string replacement_file_path = replacement_json_node.GetAbsolutePath("replacementPath");
        const std::string output_file_path = Path::Combine(*m_data->output_directory, repo_path);

        m_data->logging_list_box->AddText("Replacing: " + repo_path);

        PortableFunctions::FileCopyWithExceptions(replacement_file_path, output_file_path, FileOverwriteFlag::Fail);
    }
}


template<typename git_treeT>
void Creator::CreateSqliteWithoutSEE(const git_treeT* const tree)
{
    m_data->logging_list_box->AddText(SharableString());
    m_data->logging_list_box->AddText("Creating the non-SEE version of SQLite...");

    const std::string sqlite_repo_path = "cspro/external/SQLite/";

    git_tree_entry* tree_entry;
    const int result = git_tree_entry_bypath(&tree_entry, tree, Path::Combine(sqlite_repo_path, "sqlite3.h").c_str());

    if( result == GIT_ENOTFOUND )
        throw CSProException("Could not find the SQLite header file.");

    if( result < 0 )
        ThrowGitException();

    git_object* object;

    if( git_tree_entry_to_object(&object, git_tree_owner(tree), tree_entry) < 0 )
        ThrowGitException();

    ASSERT(git_tree_entry_type(tree_entry) == GIT_OBJECT_BLOB);

    const git_blob* const blob = reinterpret_cast<git_blob*>(object);

    const std::string_view header_sv(static_cast<const char*>(git_blob_rawcontent(blob)),
                                     static_cast<size_t>(git_blob_rawsize(blob)));

    // find the version of SQLite that this release uses
    constexpr std::string_view VersionPrefix_sv = "#define SQLITE_VERSION";
    std::string full_version_line;
    std::string version;

    SO::ForeachLine(header_sv, false,
        [&](std::string_view line_sv)
        {
            if( SO::StartsWith(line_sv, VersionPrefix_sv) )
            {
                full_version_line = line_sv;

                line_sv.remove_prefix(VersionPrefix_sv.length());
                SO::MakeTrim(line_sv);
                SO::MakeTrim(line_sv, '\"');
                version = line_sv;

                return false;
            }

            return true;
        });

    if( version.empty() )
        throw CSProException("Could not find the version in sqlite3.h.");

    git_object_free(object);

    git_tree_entry_free(tree_entry);

    m_data->logging_list_box->AddText("Found SQLite version for this release: " + version);

    // this repository hosts non-SEE SQLite amalgamations: https://github.com/rhuijben/sqlite-amalgamation/
    constexpr const char* AmalgamationRepository = "rhuijben/sqlite-amalgamation";

    CurlHttpConnection connection;

    // find this commit with this version
    std::string commit_sha;

    for( int commit_page = 1; commit_sha.empty(); ++commit_page )
    {
        HeaderList headers;
        headers.Add("User-Agent: CSPro Open Source Code Cleanup");
        headers.Add_Accept_Json();

        std::string url = FormatText("https://api.github.com/repos/%s/commits?page=%d", AmalgamationRepository, commit_page);

        const HttpRequest request = HttpRequestBuilder(std::move(url), std::move(headers)).build();
        HttpResponse response = connection.Request(request);

        const JsonNode json_node = Json::Parse(response.body.ToString());
        const JsonNodeArray commits_json_node_array = json_node.GetArray();

        if( commits_json_node_array.empty() )
            break;

        for( const JsonNode& commit_json_node : commits_json_node_array )
        {
            const std::string commit_message = commit_json_node.Get("commit")
                                                               .Get<std::string>("message");

            if( commit_message.find(version) != std::string::npos )
            {
                if( !commit_sha.empty() )
                    throw CSProException("Multiple SQLite amalgamations have a commit message containing: " + version);

                commit_sha = commit_json_node.Get<std::string>("sha");
            }
        }
    }

    if( commit_sha.empty() )
        throw CSProException("No SQLite amalgamation has a commit message containing: " + version);

    m_data->logging_list_box->AddText(FormatText("Downloading SQLite files from %s commit SHA: %s", AmalgamationRepository, commit_sha.c_str()));

    // download the non-SEE versions
    for( int i = 0; i < 2; ++i )
    {
        const bool is_header = ( i == 0 );
        const char* const filename = is_header ? "sqlite3.h" : "sqlite3.c";

        const std::string url = FormatText("https://raw.githubusercontent.com/%s/%s/%s", AmalgamationRepository, commit_sha.c_str(), filename);

        const HttpRequest request = HttpRequestBuilder(std::move(url)).build();
        HttpResponse response = connection.Request(request);

        if( response.http_status != HttpResponse::Status_200_OK )
            throw CSProException("Error accessing: " + url);

        const std::string body = response.body.ToString();

        if( is_header && body.find(full_version_line) == std::string::npos )
            throw CSProException("The SQLite amalgamation version header does not match: " + full_version_line);

        const std::string output_file_path = Path::Combine(*m_data->output_directory, Path::ToNativeSlash(sqlite_repo_path), filename);

        m_data->logging_list_box->AddText(FormatText("Saving '%s' (length %d) to: %s", filename, static_cast<int>(body.size()), output_file_path.c_str()));

        FileIO::WriteText(output_file_path, body, false);
    }
}
