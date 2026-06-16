#include "StdAfx.h"
#include "LibraryManager.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Hash.h>


CREATE_JSON_KEY(libraryId)
CREATE_JSON_KEY(localHash)
CREATE_JSON_KEY(tag)


namespace
{
    constexpr std::string_view LibrariesCommitMessageIdentifier_sv = "libraries hash: ";
    constexpr size_t LibrariesHashHexLength                        = 32;

    constexpr std::tuple<std::string_view, std::string_view> LibraryDirectoryAndWildcard_sv[] =
    {
        { "build-tools/Installer Inputs/Webview2",        "MicrosoftEdgeWebview2Setup.exe" },
        { "cspro/external",                               "*.dll;*.lib" },
        { "cspro/CSEntryDroid/app/libs",                  "*.jar" },
        { "cspro/CSEntryDroid/app/src/main/jni/external", "*.a" }
    };
}


LibraryManager::LibraryManager(Controller& controller) noexcept
    :   m_controller(controller)
{
    try
    {
        m_builds = LoadCachedBuilds();
    }

    catch(...)
    {
        ASSERT(false);
        m_builds = std::make_unique<std::vector<Build>>();
    }
}


std::unique_ptr<std::vector<LibraryManager::Build>> LibraryManager::LoadCachedBuilds() const
{
    auto builds = std::make_unique<std::vector<Build>>();

    const std::string json = m_controller.GetSettingsDb().ReadOrDefault(SettingsKeys::Libraries_sv, SO::Empty_string);

    if( !json.empty() )
    {
        const JsonNode json_node = Json::Parse(json);

        for( const JsonNode& build_json_node : json_node.GetArray() )
        {
            builds->emplace_back(
                Build
                {
                    build_json_node.Get<std::string>(JK::tag),
                    build_json_node.Get<std::string>(JK::libraryId),
                    build_json_node.GetOrConstruct<std::string>(JK::localHash)
                }
            );
        }
    }

    return builds;
}


void LibraryManager::CacheBuilds(const std::vector<Build>& builds) const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginArray();

    for( const Build& build : builds )
    {
        json_writer->BeginObject()
                    .Write(JK::tag, build.tag_name)
                    .Write(JK::libraryId, build.library_id)
                    .WriteIfNotBlank(JK::localHash, build.local_hash)
                    .EndObject();
    }

    json_writer->EndArray();

    m_controller.GetSettingsDb().Write(SettingsKeys::Libraries_sv, json_writer->GetString());
}


void LibraryManager::RefreshBuildsFromTags()
{
    GitRepository& library_repo = m_controller.GetOpenSourceLibrariesRepo();

    m_controller.LogText("Reading tags from the open source libraries repository.");

    auto builds = std::make_unique<std::vector<Build>>();

    library_repo.ForeachTag(
        [&](const GitTag tag)
        {
            std::string tag_name = tag.GetDisplayName();
            m_controller.LogText("Library tag: " + tag_name);

            // look up the commit message and see if it contains the library ID
            const GitCommit commit = library_repo.LookupCommit(tag);
            const std::string& commit_message = commit.GetMessage();

            const size_t id_pos = commit_message.find(LibrariesCommitMessageIdentifier_sv);

            if( id_pos == std::string::npos )
            {
                m_controller.LogText(u8"⚠ Library ID not found!");
            }

            else
            {
                std::string library_id(SO::Trim(
                    std::string_view(commit_message).substr(id_pos + LibrariesCommitMessageIdentifier_sv.length())
                ));

                if( library_id.length() != LibrariesHashHexLength )
                    throw CSProException("The library ID was not valid: %s", library_id.c_str());

                m_controller.LogText("Library ID updated: " + library_id);

                builds->emplace_back(Build { std::move(tag_name), std::move(library_id) });
            }

            return true;
        });

    // sort in reverse order by tag name
    std::sort(builds->begin(), builds->end(),
              [](const Build& b1, const Build& b2) { return ( b1.tag_name > b2.tag_name ); });

    CacheBuilds(*builds);
    m_builds = std::move(builds);
}


const std::vector<RepoFilePath>& LibraryManager::GetInputs()
{
    if( !m_inputRepoFilePaths.empty() )
        return m_inputRepoFilePaths;

    DirectoryLister directory_lister(true);

    const std::string& private_repo_directory = m_controller.GetPrivateRepoDirectory();
    ASSERT(private_repo_directory.back() == Path::NativeSlashChar);

    for( const auto& [directory_sv, wildcard_sv] : LibraryDirectoryAndWildcard_sv )
    {
        const std::string full_directory = Path::Combine(private_repo_directory,
                                                         Path::ToNativeSlash(std::string(directory_sv)));
        directory_lister.SetNameFilter(wildcard_sv);

        for( std::string& file_path : directory_lister.GetPaths(full_directory) )
            m_inputRepoFilePaths.emplace_back(RepoFilePath::CreateFromFilePath(std::move(file_path), private_repo_directory));
    }

    return m_inputRepoFilePaths;
}


bool operator<(const LibraryManager::Input& input1, const LibraryManager::Input& input2) noexcept
{
    if( input1.repo_file_path.repo_path == input2.repo_file_path.repo_path )
    {
        ASSERT(input1.repo_file_path.file_path == input2.repo_file_path.file_path);
        return ( input1.cs_blob_oid < input2.cs_blob_oid );
    }

    return ( input1.repo_file_path.repo_path < input2.repo_file_path.repo_path );
}


std::vector<LibraryManager::Input> LibraryManager::GetInputs(const GitCommit& cs_commit)
{
    std::vector<Input> inputs;

    // to determine the built libraries needed at this point, we will look at the
    // directories where external libraries are located, and then use the version
    // in the repository (when available), or the version on the disk (when not)
    const GitIndex cs_index = cs_commit.GetTree().GetIndex();

    for( const RepoFilePath& repo_file_path : GetInputs() )
    {
        Input& input = inputs.emplace_back(Input { repo_file_path });

        try
        {
            // an exception is thrown if this library in not in repository
            input.cs_blob_oid = cs_index.GetObjectIdByPath(repo_file_path.repo_path);
        }
        catch(...) { }
    }

    return inputs;
}


struct LibraryManager::FileData
{
    BinaryBlock file_data;
    std::string md5;
};


std::string LibraryManager::CalculateCacheKey(const std::vector<Input>& inputs, const bool local_version)
{
    std::string cache_key_inputs;

    for( const Input& input : inputs )
    {
        cache_key_inputs.append(input.repo_file_path.repo_path);

        // the MD5 may have already been calculated
        auto lookup = m_fileData.find(input);

        auto process_file_data = [&](BinaryBlock binary_block)
        {
            ASSERT(lookup == m_fileData.cend());

            std::unique_ptr<FileData> file_data(new FileData { std::move(binary_block) });
            file_data->md5 = Hash::Md5::Create(file_data->file_data);
            return m_fileData.try_emplace(input, std::move(file_data)).first;
        };

        // for files in the repository, the cache key will be the OID hex hash
        // for the local version and the MD5 for the actual version
        if( input.cs_blob_oid.has_value() )
        {
            if( local_version )
            {
                cache_key_inputs.append(input.cs_blob_oid->GetHexHash());
            }

            else
            {
                if( lookup == m_fileData.cend() )
                {
                    const GitBlob cs_blob = m_controller.GetPrivateRepo().LookupBlob(*input.cs_blob_oid);
                    lookup = process_file_data(BinaryBlock(cs_blob.data(), cs_blob.size()));
                }

                cache_key_inputs.append(lookup->second->md5);
            }
        }

        // for files on disk, the cache key will be the file size and modified time
        // for the local version and the MD5 for the actual version
        else
        {
            if( local_version )
            {
                const std::tuple<int64_t, int64_t> file_size_and_modified_time = PortableFunctions::FileSizeAndModifiedTime(input.repo_file_path.file_path);
                cache_key_inputs.append(IntToString(std::get<0>(file_size_and_modified_time)));
                cache_key_inputs.append(IntToString(std::get<1>(file_size_and_modified_time)));
            }

            else
            {
                if( lookup == m_fileData.cend() )
                    lookup = process_file_data(FileIO::ReadBinary(input.repo_file_path.file_path));

                cache_key_inputs.append(lookup->second->md5);
            }
        }
    }

    return Hash::Create(cache_key_inputs, LibrariesHashHexLength / 2);
}


void LibraryManager::CreateAndCommitBuild(const GitCommit& cs_commit)
{
    GitRepository& library_repo = m_controller.GetOpenSourceLibrariesRepo();

    if( library_repo.HasChanges() )
        throw CSProException("You cannot create a library commit if there are changes in the open source libraries directory.");

    m_controller.LogText("Creating a commit with the built libraries as of:\n    %s\n    %s",
                         cs_commit.GetCommitter().GetWhen().GetLocalDateTimeString().c_str(),
                         cs_commit.GetMessage().c_str());

    const std::vector<Input> inputs = GetInputs(cs_commit);
    std::string library_id = CalculateCacheKey(inputs, false);
    std::string local_hash = CalculateCacheKey(inputs, true);

    const int64_t commit_timestamp = cs_commit.GetCommitter().GetWhen().GetTimestamp();
    const DateTime::Components date_time_components = DateTime::TimeToComponents(commit_timestamp);
    const std::string commit_yyyy_mm_dd = FormatText("%04d-%02d-%02d",
        date_time_components.year, date_time_components.month, date_time_components.day
    );

    // the tag name will be the commit date and the first seven characters of the source commit's OID
    const std::string cs_commit_oid_hash = cs_commit.GetObjectId().GetHexHash();
    std::string tag_name = FormatText("v%s-%.7s", commit_yyyy_mm_dd.c_str(), cs_commit_oid_hash.c_str());

    if( library_repo.IsTag(tag_name) )
        throw CSProException("A library commit for these libraries already exists, tagged with: " + tag_name);

    // all commits will be to main
    GitBranch branch = library_repo.LookupBranch("main");
    library_repo.CheckoutBranch(branch);

    GitIndex index = library_repo.GetIndex();
    const std::map<std::string, GitObjectId> current_files = index.GetPathObjectIdMap();

    // remove any libraries no longer used
    for( const auto& [repo_path, oid] : current_files )
    {
        // ignore any files at the root (e.g., README.md)
        if( PortableFunctions::PathGetDirectory(repo_path).empty() )
            continue;

        const auto& lookup = std::find_if(inputs.cbegin(), inputs.cend(),
            [&](const Input& input) { return ( repo_path == input.repo_file_path.repo_path ); }
        );

        if( lookup == inputs.cend() )
        {
            m_controller.LogText("Removing library: " + repo_path);
            index.RemoveEntryByPath(repo_path);
        }
    }

    // add the current set of external libraries
    for( const Input& input : inputs )
    {
        const std::string& repo_path = input.repo_file_path.repo_path;

        // only add the entry if it has not been previously added, or has changed
        const auto& lookup = current_files.find(repo_path);

        if( lookup != current_files.cend() &&
            lookup->second == input.cs_blob_oid )
        {
            m_controller.LogText("Library is unchanged: " + repo_path);
        }

        else
        {
            m_controller.LogText("Adding library: " + repo_path);

            const auto& file_data_lookup = m_fileData.find(input);
            ASSERT(file_data_lookup != m_fileData.cend());

            const GitObjectId blob_oid = library_repo.CreateBlob(file_data_lookup->second->file_data);
            index.AddEntry(blob_oid, repo_path, GIT_FILEMODE_BLOB);
        }
    }

    // for the signature, use "CSPro Bot" with the date of the source commit
    GitSignature author_and_committer = Controller::GetCSProBotSignature(library_repo);
    author_and_committer.SetWhen(cs_commit.GetCommitter().GetWhen());

    // the message will contain the source commit's date and OID, and then the library ID
    const std::string message = SO::Concatenate(
        "libraries as of ", commit_yyyy_mm_dd,
        "\n\nsource commit: https://github.com/CSProDevelopment/cspro/commit/", cs_commit_oid_hash,
        "\n\n", LibrariesCommitMessageIdentifier_sv, library_id
    );

    // make sure that there are actually differences
    const GitCommit parent_commit = library_repo.LookupCommit(branch);
    GitTree parent_tree = parent_commit.GetTree();

    GitTree tree = library_repo.WriteTree(index);

    const GitDiff merge_diff = m_controller.GetOpenSourceRepo().GetDifference(parent_tree, tree);

    if( merge_diff.GetNumberDeltas() == 0 )
    {
        throw CSProException("There are no library changes compared to the previous commit: " +
                             parent_commit.GetObjectId().GetHexHash());
    }

    // create the commit
    const GitObjectId commit_oid = library_repo.CreateCommit(
        author_and_committer,
        message,
        tree,
        parent_commit
    );

    m_controller.LogText("Build library created: " + commit_oid.GetHexHash());

    // tag the commit, with the message referencing the source commit's OID
    library_repo.CreateTag(
        author_and_committer,
        library_repo.LookupCommit(commit_oid),
        tag_name,
        "Created from: " + cs_commit_oid_hash
    );

    m_controller.LogText("Build library tagged: " + tag_name);

    // when complete, checkout the HEAD so that the working directory matches the index
    library_repo.CheckoutHead(GIT_CHECKOUT_FORCE | GIT_CHECKOUT_REMOVE_UNTRACKED);

    // cache this build
    auto builds = std::make_unique<std::vector<Build>>(*GetBuilds());

    builds->insert(builds->begin(),
        Build
        {
            std::move(tag_name),
            std::move(library_id),
            std::move(local_hash)
        }
    );

    CacheBuilds(*builds);
    m_builds = std::move(builds);
}


std::string LibraryManager::GetTagForBuiltLibraries(const GitCommit& cs_commit)
{
    const std::vector<Input> inputs = GetInputs(cs_commit);
    const std::shared_ptr<const std::vector<Build>> builds = GetBuilds();

    // first see if the tag has been cached using the local cache key
    std::string local_hash = CalculateCacheKey(inputs, true);

    auto lookup = std::find_if(builds->cbegin(), builds->cend(),
        [&](const Build& build) { return ( local_hash == build.local_hash ); }
    );

    // if not, check if tag has been cached using the actual library ID
    if( lookup == builds->cend() )
    {
        const std::string library_id = CalculateCacheKey(inputs, false);

        lookup = std::find_if(builds->cbegin(), builds->cend(),
            [&](const Build& build) { return ( library_id == build.library_id ); }
        );

        if( lookup == builds->cend() )
        {
            throw CSProException("Refresh the library IDs and try again.\n"
                                 "On failure, commit a built library for:\n\n" +
                                 cs_commit.GetObjectId().GetHexHash());
        }

        // add the local hash to the cache for future use
        auto updated_builds = std::make_unique<std::vector<Build>>(*builds);
        updated_builds->at(std::distance(builds->cbegin(), lookup)).local_hash = std::move(local_hash);

        CacheBuilds(*updated_builds);
        m_builds = std::move(updated_builds);
    }

    return lookup->tag_name;
}
