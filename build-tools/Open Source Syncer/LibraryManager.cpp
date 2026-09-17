#include "StdAfx.h"
#include "LibraryManager.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Hash.h>
#include <zGit/GitIgnoreEvaluator.h>
#include <zZip/ZipFile.h>


CREATE_JSON_KEY(architecture)
CREATE_JSON_KEY(hash)
CREATE_JSON_KEY(libraries)
CREATE_JSON_KEY(platform)
CREATE_JSON_KEY(targets)


namespace
{
    constexpr bool IncludeWasmLibraries = true;

    constexpr size_t LibrariesIdTagLength = 10;

    constexpr const char* ReleaseNotesTemplateFilename = "GitHubThirdPartyLibrariesRelease.md";

    constexpr const char* LibrariesDataRepoPath = "third_party/prebuilt/libraries.json";
    constexpr const char* ExternalLibrariesMarkdownRepoPath = "docs/external-libraries.md";
    constexpr const char* ThirdPartyPrebuiltRepoPath = "third_party/prebuilt";

    const std::map<std::string, const char*> AdditionalLibraryRepoPaths =
    {
        { "windows", "build-tools/Installer Inputs/Webview2/MicrosoftEdgeWebview2Setup.exe" },
        { "android", "cspro/CSEntryDroid/app/libs/*.jar" },
    };

    constexpr std::string_view Fill_LibrariesId_sv           = "~~LIBRARIES_ID~~";
    constexpr std::string_view Fill_LibrariesCommit_sv       = "~~LIBRARIES_COMMIT~~";
    constexpr std::string_view Fill_LibrariesCommitShort_sv  = "~~LIBRARIES_COMMIT_SHORT~~";
    constexpr size_t           Fill_CommitShortLength         = 10;
}


LibraryManager::LibraryManager(Controller& controller) noexcept
    :   m_controller(controller)
{
    try
    {
        m_librariesDataFilePath = CreatePathFromRepoPath(LibrariesDataRepoPath);
        LoadCachedFileHashes();
    }
    catch(...) { ASSERT(false); }
}


LibraryManager::~LibraryManager()
{
}


GitHubRepositoryConnection& LibraryManager::GetThirdPartyLibrariesGitHubRepositoryConnection()
{
    if( m_ghConnection == nullptr )
        m_ghConnection = std::make_unique<GitHubRepositoryConnection>("csprousers", "cspro-libraries-third-party");

    return *m_ghConnection;
}


std::string LibraryManager::CreatePathFromRepoPath(std::string repo_file_path) const
{
    return Path::Combine(m_controller.GetPrivateRepoDirectory(),
                         Path::MakeToNativeSlash(repo_file_path));
}


std::string LibraryManager::CreateRepoPathFromPath(const std::string& path) const
{
    const std::string& private_repo_directory = m_controller.GetPrivateRepoDirectory();
    ASSERT(private_repo_directory.back() == Path::NativeSlashChar);
    ASSERT(SO::StartsWith(path, private_repo_directory));

    return Path::ToForwardSlash(path.substr(private_repo_directory.length()));
}


void LibraryManager::LoadCachedFileHashes()
{
    const std::string json = m_controller.GetSettingsDb().ReadOrDefault(SettingsKeys::FileHashes_sv, SO::Empty_string);

    if( json.empty() )
        return;

    const JsonNode json_node = Json::Parse(json);

    for( const JsonNode& file_json_node : json_node.GetArray() )
    {
        m_fileHashes.emplace_back(FileHash {
            file_json_node.Get<std::string>(JK::path),
            file_json_node.Get<std::string>(JK::hash),
            file_json_node.Get<int64_t>(JK::size),
            file_json_node.Get<int64_t>(JK::modifiedTime)
        });
    }
}


void LibraryManager::SaveCachedFileHashes() const
{
    ASSERT(!m_fileHashes.empty());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->WriteObjects(m_fileHashes,
        [&](const FileHash& file_hash)
        {
            json_writer->Write(JK::path, file_hash.file_path)
                        .Write(JK::hash, file_hash.sha256)
                        .Write(JK::size, file_hash.file_size)
                        .Write(JK::modifiedTime, file_hash.file_modified_time);
        });

    m_controller.GetSettingsDb().Write(SettingsKeys::FileHashes_sv, json_writer->GetString());
}


std::vector<LibraryManager::FileHash> LibraryManager::CalculateFileHashes(const std::vector<std::string>& file_paths)
{
    std::vector<FileHash> file_hashes;

    const size_t count_initial_cached_file_hashes = m_fileHashes.size();

    for( const std::string& file_path : file_paths )
    {
        int64_t file_size;
        int64_t file_modified_time;
        std::tie(file_size, file_modified_time) = PortableFunctions::FileSizeAndModifiedTime<true>(file_path);

        const auto& lookup = std::find_if(
            m_fileHashes.cbegin(), m_fileHashes.cend(),
            [&](const FileHash& file_hash)
            {
                return ( file_hash.file_path == file_path &&
                         file_hash.file_size == file_size &&
                         file_hash.file_modified_time == file_modified_time );
            });

        // use a cached SHA-256
        if( lookup != m_fileHashes.cend() )
        {
            file_hashes.emplace_back(*lookup);
        }

        // or calculate a new one
        else
        {
            m_fileHashes.emplace_back(FileHash {
                file_path,
                Hash::Sha256::CreateFromFile(file_path, true),
                file_size,
                file_modified_time
            });

            file_hashes.emplace_back(m_fileHashes.back());
        }
    }

    // if any new hashes were calculated, cache them
    if( count_initial_cached_file_hashes != m_fileHashes.size() )
        SaveCachedFileHashes();

    return file_hashes;
}


LibraryManager::LibrariesData LibraryManager::LoadLibrariesData() const
{
    const JsonNode json_node = Json::ParseFile(m_librariesDataFilePath);
    return ParseLibrariesData(json_node);
}


LibraryManager::LibrariesData LibraryManager::ParseLibrariesData(const JsonNode& json_node) const
{
    LibrariesData libraries_data;

    libraries_data.id = json_node.Get<std::string>(JK::id);

    for( const JsonNode& library_json_node : json_node.GetArrayOrEmpty(JK::libraries) )
    {
        libraries_data.library_versions.emplace_back(LibraryVersion {
            library_json_node.Get<std::string>(JK::name),
            library_json_node.Get<std::string>(JK::version)
        });
    }

    for( const JsonNode& target_json_node : json_node.GetArrayOrEmpty(JK::targets) )
    {
        LibrariesData::Target& target = libraries_data.targets.emplace_back(LibrariesData::Target {
            target_json_node.Get<std::string>(JK::platform),
            target_json_node.Get<std::string>(JK::platform),
            target_json_node.Get<std::string>(JK::architecture)
        });

        if( !target.architecture.empty() )
            SO::AppendWithSeparator(target.library_type, target.architecture, '-');

        for( const JsonNode& file_json_node : target_json_node.GetArrayOrEmpty(JK::files) )
        {
            target.files.emplace_back(FileHash {
                CreatePathFromRepoPath(file_json_node.Get<std::string>(JK::path)),
                file_json_node.Get<std::string>(JK::hash),
                -1, // file_size
                -1 // file_modified_time
            });
        }
    }

    return libraries_data;
}


void LibraryManager::SaveLibrariesData(const LibrariesData& libraries_data) const
{
    const std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(m_librariesDataFilePath);

    json_writer->BeginObject()
                .Write(JK::id, libraries_data.id);

    json_writer->WriteArray(JK::libraries, libraries_data.library_versions,
        [&](const LibraryVersion& library_version)
        {
            const JsonWriter::FormattingHolder json_formatting_holder = json_writer->SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing);

            json_writer->BeginObject();
            json_writer->SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

            json_writer->Write(JK::name, library_version.name)
                        .Write(JK::version, library_version.version)
                        .EndObject();
        });

    json_writer->WriteObjects(JK::targets, libraries_data.targets,
        [&](const LibrariesData::Target& target)
        {
            json_writer->Write(JK::platform, target.platform)
                        .Write(JK::architecture, target.architecture);

            json_writer->WriteArray(JK::files, target.files,
                [&](const FileHash& file_hash)
                {
                    const JsonWriter::FormattingHolder json_formatting_holder = json_writer->SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing);

                    json_writer->BeginObject();
                    json_writer->SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

                    json_writer->Write(JK::path, CreateRepoPathFromPath(file_hash.file_path))
                                .Write(JK::hash, file_hash.sha256)
                                .EndObject();
                });
        });

    json_writer->EndObject();
}


void LibraryManager::CalculateLibrariesId(LibrariesData& libraries_data) const
{
    std::string cache_key_inputs;

    for( const LibrariesData::Target& target : libraries_data.targets )
    {
        for( const FileHash& file_hash : target.files )
        {
            cache_key_inputs.append(CreateRepoPathFromPath(file_hash.file_path))
                            .append(file_hash.sha256);
        }
    }

    libraries_data.id = Hash::Sha256::Create(cache_key_inputs);
}


LibraryManager::LibrariesData LibraryManager::UpdateLibrariesData(const bool save_library_data)
{
    const std::vector<std::string> library_types =
    {
        // Windows
        "windows-x86",
        "windows-x64",

        // Android
        "android-arm64-v8a",
        "android-armeabi-v7a",
        "android-x86_64",

        // WASM
        "wasm"
    };

    return UpdateLibrariesData(library_types, save_library_data);
}



LibraryManager::LibrariesData LibraryManager::UpdateLibrariesData(const std::vector<std::string>& library_types,
                                                                  const bool save_library_data)
{
    // load the current libraries data
    LibrariesData libraries_data = LoadLibrariesData();

    // update the library versions
    libraries_data.library_versions = ReadLibraryVersions();

    // update the files for this library type
    for( const std::string& library_type : library_types )
    {
        auto library_type_lookup = std::find_if(
            libraries_data.targets.begin(), libraries_data.targets.end(),
            [&](const LibrariesData::Target& target) { return ( target.library_type == library_type ); }
        );

        // add a new target when it does not exist in the previously saved data
        if( library_type_lookup == libraries_data.targets.end() )
        {
            LibrariesData::Target& target = libraries_data.targets.emplace_back(LibrariesData::Target { library_type });
            std::tie(target.platform, target.architecture) = SO::GetTextOnEitherSideOfCharacter(library_type, '-');
            library_type_lookup = libraries_data.targets.end() - 1;
        }

        // update the files
        library_type_lookup->files = CalculateFileHashes(
            GetLibraryTargetFilePaths(library_type, library_type_lookup->platform)
        );
    }

    // update the libraries ID
    CalculateLibrariesId(libraries_data);

    // potentially save the updated data
    if( save_library_data )
        SaveLibrariesData(libraries_data);

    return libraries_data;
}


std::vector<LibraryManager::LibraryVersion> LibraryManager::ReadLibraryVersions() const
{
    const std::string external_libraries_doc = FileIO::ReadText(
        CreatePathFromRepoPath(ExternalLibrariesMarkdownRepoPath)
    );

    // use regular expressions to extract the library names and versions
    const std::regex library_name_regex(R"(^### (.+)$)");
    const std::regex library_version_regex(R"(^\*Current version: (\d.*\d).*\*$)");
    std::smatch matches;

    std::vector<LibraryVersion> library_versions;
    std::string last_library_name;

    SO::ForeachLine<std::string>(external_libraries_doc, false,
        [&](const std::string& line)
        {
            if( std::regex_match(line, matches, library_name_regex) )
            {
                last_library_name = matches.str(1);
            }

            else if( std::regex_match(line, matches, library_version_regex) )
            {
                if( last_library_name.empty() )
                    throw CSProException("Missing library name for line: " + line);

                library_versions.emplace_back(LibraryVersion { std::move(last_library_name), matches.str(1) });
                ASSERT(last_library_name.empty());
            }
        });

    return library_versions;
}


std::vector<std::string> LibraryManager::GetLibraryTargetFilePaths(const std::string& library_type, const std::string& platform) const
{
    // add the libraries in the third_party/prebuilt directory
    std::vector<std::string> library_file_paths = GetPrebuiltLibraryFilePaths(library_type);

    if( library_file_paths.empty() )
        throw CSProException("The library type is not valid: " + library_type );

    // add libraries in other directories
    const auto& additional_libraries_lookup = AdditionalLibraryRepoPaths.find(platform);

    if( additional_libraries_lookup != AdditionalLibraryRepoPaths.cend() )
    {
        DirectoryLister directory_lister;
        const std::string directory_with_wildcard = CreatePathFromRepoPath(additional_libraries_lookup->second);

        for( std::string& file_path : directory_lister.GetFilePathsWithPossibleWildcard(directory_with_wildcard, true) )
            library_file_paths.emplace_back(std::move(file_path));
    }

    // sort by file path
    std::sort(library_file_paths.begin(), library_file_paths.end());

    return library_file_paths;
}


std::vector<std::string> LibraryManager::GetPrebuiltLibraryFilePaths(const std::string& library_type) const
{
    const std::string prebuilt_directory = CreatePathFromRepoPath(ThirdPartyPrebuiltRepoPath);

    // libraries are specified as files to ignore
    const std::string prebuild_gitignore_file_path = Path::Combine(prebuilt_directory, ".gitignore");

    GitIgnoreEvaluator exclusion_evaluator;
    exclusion_evaluator.AddRulesFromFile(prebuild_gitignore_file_path);

    DirectoryLister directory_lister;
    std::vector<std::string> file_paths;

    for( const char* const directory_type : { "bin", "lib" } )
    {
        const std::string directory = Path::Combine(prebuilt_directory, directory_type, library_type);

        directory_lister.ForeachPath(directory,
            [&](const std::string& file_path)
            {
                if( exclusion_evaluator.Ignore(file_path) )
                    file_paths.emplace_back(file_path);

                return true;
            });
    }

     return file_paths;
}


std::set<std::string> LibraryManager::GetThirdPartyFilePaths()
{
    LibrariesData libraries_data = UpdateLibrariesData(false);

    std::set<std::string> file_paths;

    for( LibrariesData::Target& target : libraries_data.targets )
    {
        for( FileHash& file_hash : target.files )
            file_paths.insert(std::move(file_hash.file_path));
    }

    return file_paths;
}


std::string LibraryManager::GetLibrariesId(const LibrariesIdType libraries_id_type)
{
    switch( libraries_id_type )
    {
        case LibrariesIdType::InLibrariesJson:
            return LoadLibrariesData().id;

        case LibrariesIdType::CalculatedFromWorkingDirectory:
            return UpdateLibrariesData(false).id;

        default:
            throw ProgrammingErrorException();
    }
}


std::string LibraryManager::GetLibrariesId(const GitCommit& cs_commit)
{
    const GitTree current_tree = cs_commit.GetTree();
    const GitTreeEntry tree_entry = current_tree.GetEntryByPath(LibrariesDataRepoPath);
    const GitBlob blob = tree_entry.GetObject().GetBlob();

    const JsonNode json_node = Json::Parse(blob.as<std::string_view>());

    return ParseLibrariesData(json_node).id;
}


std::string LibraryManager::GetLibrariesReleaseTag(const std::string& libraries_id)
{
    // find the libraries ID locally
    GitRepository& libraries_repo = m_controller.GetThirdPartyLibrariesRepo();

    const std::string libraries_id_in_tag_name = libraries_id.substr(0, LibrariesIdTagLength);
    std::string tag_name;

    libraries_repo.ForeachTag(
        [&](const GitTag tag)
        {
            if( tag.GetName().find(libraries_id_in_tag_name) != std::string::npos )
            {
                tag_name = tag.GetDisplayName();
                return false;
            }

            return true;
        });

    if( tag_name.empty() )
    {
        throw CSProException("No tag exists in the local repository that matches libraries ID '%s'.\n\n"
                             "Fetch any tags before proceeding.",
                             libraries_id.c_str());
    }

    GitHubRepositoryConnection& gh_connection = GetThirdPartyLibrariesGitHubRepositoryConnection();

    if( gh_connection.GetTag(tag_name).empty() )
        throw CSProException("No tag exists on GitHub that matches libraries release tag '%s'.", tag_name.c_str());

    return tag_name;
}


void LibraryManager::CreateLibraryRelease()
{
    const std::string saved_libraries_id = GetLibrariesId(LibrariesIdType::InLibrariesJson);
    const LibrariesData libraries_data = UpdateLibrariesData(true);

    m_controller.LogText("Creating a release for libraries ID: " + libraries_data.id);

    if( libraries_data.id != saved_libraries_id )
    {
        throw CSProException("The working directory's libraries.json does not match the current libraries ID. "
                             "Validate the newly saved file and try again.");
    }

    // make sure that the latest libraries.json is committed
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    const GitCommit cs_current_commit = private_repo.LookupCommit(private_repo.GetCurrentBranch());

    if( libraries_data.id != GetLibrariesId(cs_current_commit) )
    {
        throw CSProException("The most recently committed libraries.json does not match the current libraries ID. "
                             "Commit the file before proceeding.");
    }

    // the release will be created based on a third-party libraries repository tag
    GitRepository& libraries_repo = m_controller.GetThirdPartyLibrariesRepo();

    if( libraries_repo.HasChanges() )
        throw CSProException("You cannot create a library release if there are changes in the third-party libraries directory.");

    const GitCommit libraries_commit = libraries_repo.LookupCommit(libraries_repo.GetCurrentBranch());
    const std::string libraries_commit_oid_hash = libraries_commit.GetObjectId().GetHexHash();

    m_controller.LogText("The third-party libraries repository will be tagged at commit: " + libraries_commit_oid_hash);

    // the tag name will be the commit date and the first 10 characters of the libraries ID
    std::string tag_name;

    GitRevisionWalker walker(private_repo);
    walker.WalkFileRevisions(LibrariesDataRepoPath, cs_current_commit,
        [&](const GitCommit commit)
        {
            if( libraries_data.id != GetLibrariesId(commit) )
                throw ProgrammingErrorException();

            const int64_t commit_timestamp = commit.GetCommitter().GetWhen().GetTimestamp();
            const DateTime::Components date_time_components = DateTime::TimeToComponents(commit_timestamp);

            tag_name = FormatText(
                "v%04d-%02d-%02d-%.*s",
                date_time_components.year, date_time_components.month, date_time_components.day,
                static_cast<int>(LibrariesIdTagLength), libraries_data.id.c_str()
            );

            return false;
        });

    if( tag_name.empty() )
        throw ProgrammingErrorException();

    // create the release notes
    std::string release_notes = CreateReleaseNotes(libraries_data.id, libraries_commit_oid_hash);

    // create the release assets
    const std::vector<std::tuple<std::string, std::shared_ptr<const BinaryBlock>>> release_assets =
        CreateReleaseAssets(libraries_data, tag_name);

    // create the lightweight tag
    GitHubRepositoryConnection& gh_connection = GetThirdPartyLibrariesGitHubRepositoryConnection();
    m_controller.LogText("Creating the GitHub tag: " + libraries_commit_oid_hash);
    gh_connection.CreateTag(tag_name, libraries_commit_oid_hash);

    // create the release
    m_controller.LogText("Creating the GitHub release with %zu assets.", release_assets.size());
    const int64_t release_id = gh_connection.CreateRelease(tag_name, tag_name, std::move(release_notes), false, release_assets);

    m_controller.LogText("GitHub release successfully created with ID " Formatter_int64_t ".", release_id);
}


std::vector<std::tuple<std::string, std::shared_ptr<const BinaryBlock>>> LibraryManager::CreateReleaseAssets(
    const LibrariesData& libraries_data, const std::string& tag_name) const
{
    std::vector<std::tuple<std::string, std::shared_ptr<const BinaryBlock>>> release_assets;

    for( const LibrariesData::Target& target : libraries_data.targets )
    {
        if( !IncludeWasmLibraries && target.platform == "wasm" )
            continue;

        std::string zip_filename = FormatText("cspro-libraries-%s-%s.zip", tag_name.c_str(), target.library_type.c_str());
        std::vector<std::string> file_paths;
        std::vector<std::string> file_paths_in_zip;

        for( const FileHash& file_hash : target.files )
        {
            file_paths.emplace_back(file_hash.file_path);
            file_paths_in_zip.emplace_back(CreateRepoPathFromPath(file_hash.file_path));
        }

        m_controller.LogText("Creating '%s' with %zu files.", zip_filename.c_str(), file_paths.size());

        TemporaryFile temporary_file;

        ZipCreator zip_creator(temporary_file.GetPath());
        zip_creator.AddFiles(file_paths, file_paths_in_zip);
        zip_creator.Close();

        release_assets.emplace_back(
            std::move(zip_filename),
            std::make_unique<BinaryBlock>(FileIO::ReadBinary(temporary_file.GetPath()))
        );
    }

    return release_assets;
}


std::string LibraryManager::CreateReleaseNotes(const std::string& libraries_id, const std::string& libraries_commit_oid_hash) const
{
    std::string release_notes = FileIO::ReadText(Controller::GetTemplatesFilePath(ReleaseNotesTemplateFilename));

    SO::RecursiveReplace(release_notes, Fill_LibrariesId_sv, libraries_id);
    SO::RecursiveReplace(release_notes, Fill_LibrariesCommit_sv, libraries_commit_oid_hash);
    SO::RecursiveReplace(release_notes, Fill_LibrariesCommitShort_sv, std::string_view(libraries_commit_oid_hash).substr(0, Fill_CommitShortLength));

    return release_notes;
}
