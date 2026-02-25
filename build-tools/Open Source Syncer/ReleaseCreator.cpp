#include "StdAfx.h"
#include "ReleaseCreator.h"
#include "LibraryManager.h"
#include "TagSyncerView.h"
#include <zToolsO/Encoders.h>


namespace
{
    constexpr const char* ReleaseNotesFilename = "GitHubRelease.md";

    constexpr std::string_view Fill_ReleasePrefix_sv          = "~~RELEASE_PREFIX~~";
    constexpr std::string_view Fill_LibrariesTag_sv           = "~~LIBRARIES_TAG~~";
    constexpr std::string_view Fill_OpenSourceCommit_sv       = "~~OPEN_SOURCE_COMMIT~~";
    constexpr std::string_view Fill_OpenSourceCommitShort_sv  = "~~OPEN_SOURCE_COMMIT_SHORT~~";
    constexpr std::string_view Fill_PrivateRepoCommit_sv      = "~~PRIVATE_REPO_COMMIT~~";
    constexpr std::string_view Fill_PrivateRepoCommitShort_sv = "~~PRIVATE_REPO_COMMIT_SHORT~~";
    constexpr std::string_view Fill_VersionFull_sv            = "~~VERSION_FULL~~";
    constexpr std::string_view Fill_VersionMajor_sv           = "~~VERSION_MAJOR~~";
    constexpr std::string_view Fill_VersionMinor_sv           = "~~VERSION_MINOR~~";
    constexpr size_t           Fill_CommitShortLength         = 10;
}


ReleaseCreator::ReleaseCreator(Controller& controller, SharableString tag_name)
    :   m_controller(controller),
        m_tagName(std::move(tag_name)),
        m_prerelease(ParseReleaseType(*m_tagName) != ReleaseType::Release),
        m_releaseNotes(FileIO::ReadText(Controller::GetTemplatesFilePath(ReleaseNotesFilename)))
{
    ASSERT(!m_controller.IsOperationRunning());

    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();
    LibraryManager& library_manager = controller.GetLibraryManager();

    // get the open source commit for this tag...
    const GitTag os_tag = open_source_repo.LookupTag(*m_tagName);
    const GitCommit os_commit = open_source_repo.LookupCommit(os_tag);
    m_openSourceCommitSHA = os_commit.GetObjectId().GetHexHash();
    m_openSourceTree = os_commit.GetTree();

    // ...and link it to the private commit
    m_privateCommitSHA = TagSyncerView::ExtractCommitSHAFromTagMessage(os_tag.GetMessage(open_source_repo));

    if( m_privateCommitSHA.empty() )
        throw CSProException("The open source tag must have a link to the private commit.");

    // get the libraries tag for this point
    const GitCommit cs_commit = private_repo.LookupCommit(m_privateCommitSHA);

    m_librariesTag = library_manager.GetTagForBuiltLibraries(cs_commit);

    // get the version at this point
    ParseVersion();
}


ReleaseType ReleaseCreator::ParseReleaseType(const std::string& tag_name)
{
    for( ReleaseType release_type = ReleaseType::Alpha;
         release_type != ReleaseType::Release;
         release_type = static_cast<ReleaseType>(static_cast<int>(release_type) + 1) )
    {
        const char* const identifier = Versioning::GetReleaseIdentifier(release_type, true);

        if( tag_name.find(identifier) != std::string::npos )
            return release_type;
    }

    return ReleaseType::Release;
}


std::string ReleaseCreator::GetReleaseFilenameIdentifier()
{
    const ReleaseType release_type = ParseReleaseType(*m_tagName);
    std::string release_filename_id = Versioning::GetReleaseIdentifier(release_type, true);

    if( release_filename_id.empty() )
    {
        ASSERT(release_type == ReleaseType::Release);
    }

    // add the release date to differentiate between multiple prereleases
    else
    {
        const std::string version_cpp = GetFileInOpenSourceRepository("cspro/zUtilO/Versioning.cpp")->as<std::string>();

        std::regex version_regex(R"((?:CSProReleaseDate.*=.*)(\d{8}))");
        std::smatch matches;

        if( !std::regex_search(version_cpp, matches, version_regex) )
            throw CSProException("Could not parse the version date.");

        release_filename_id.push_back('-');
        release_filename_id.append(matches.str(1));
    }

    return release_filename_id;
}


void ReleaseCreator::ParseVersion()
{
    ASSERT(m_versionText->empty());

    const std::string version_h = GetFileInOpenSourceRepository("cspro/zUtilO/Versioning.h")->as<std::string>();

    std::regex version_regex(R"((?:NumberDetailedText).+\"((\d+)\.(\d+)\.(\d+))\")");
    std::smatch matches;

    if( !std::regex_search(version_h, matches, version_regex) )
        throw CSProException("Could not parse the version information.");

    m_versionText = matches.str(1);

    std::get<0>(m_version) = std::stoi(matches.str(2));
    std::get<1>(m_version) = std::stoi(matches.str(3));
    std::get<2>(m_version) = std::stoi(matches.str(4));
}


void ReleaseCreator::SetAsset(std::string filename, std::shared_ptr<const BinaryBlock> data)
{
    ASSERT(filename == Path::CreateValidFilename(filename));
    ASSERT(data != nullptr && !data->empty());

    m_assets.insert_or_assign(std::move(filename), std::move(data));
}


std::shared_ptr<const BinaryBlock> ReleaseCreator::GetFileInOpenSourceRepository(const std::string& os_repo_path)
{
    ASSERT(m_openSourceTree.has_value());

    // loaded files will be cached
    auto lookup = m_loadedRepoFiles.find(os_repo_path);

    if( lookup == m_loadedRepoFiles.cend() )
    {
        const GitTreeEntry tree_entry = m_openSourceTree->GetEntryByPath(os_repo_path);
        const GitBlob blob = tree_entry.GetObject().GetBlob();

        lookup = m_loadedRepoFiles.try_emplace(
            os_repo_path,
            std::make_unique<BinaryBlock>(blob.data(), blob.size())
        ).first;
    }

    return lookup->second;
}


std::string ReleaseCreator::GetFormattedReleaseNotes() const
{
    constexpr const char* PrereleaseTagsText = "alpha, beta, or rc";
    const ReleaseType release_type = ParseReleaseType(*m_tagName);

    if( m_prerelease )
    {
        if( release_type == ReleaseType::Release )
            throw CSProException("Prerelease tags should contain one of: %s", PrereleaseTagsText);
    }

    else if( release_type != ReleaseType::Release )
    {
        throw CSProException("Release tags should not contain one of: %s", PrereleaseTagsText);
    }

    const char* const release_prefix =
        ( release_type == ReleaseType::Alpha )            ? "an alpha" :
        ( release_type == ReleaseType::Beta )             ? "a beta" :
        ( release_type == ReleaseType::ReleaseCandidate ) ? "a release candidate" :
      /*( release_type == ReleaseType::Release )*/          "a";

    std::string release_notes = m_releaseNotes;

    SO::RecursiveReplace(release_notes, Fill_ReleasePrefix_sv, release_prefix);
    SO::RecursiveReplace(release_notes, Fill_LibrariesTag_sv, *m_librariesTag);
    SO::RecursiveReplace(release_notes, Fill_OpenSourceCommit_sv, m_openSourceCommitSHA);
    SO::RecursiveReplace(release_notes, Fill_OpenSourceCommitShort_sv, std::string_view(m_openSourceCommitSHA).substr(0, Fill_CommitShortLength));
    SO::RecursiveReplace(release_notes, Fill_PrivateRepoCommit_sv, m_privateCommitSHA);
    SO::RecursiveReplace(release_notes, Fill_PrivateRepoCommitShort_sv, std::string_view(m_privateCommitSHA).substr(0, Fill_CommitShortLength));
    SO::RecursiveReplace(release_notes, Fill_VersionFull_sv, *m_versionText);
    SO::RecursiveReplace(release_notes, Fill_VersionMajor_sv, IntToString(std::get<0>(m_version)));
    SO::RecursiveReplace(release_notes, Fill_VersionMinor_sv, IntToString(std::get<1>(m_version)));

    return release_notes;
}


void ReleaseCreator::CreateRelease()
{
    // create the release as a draft
    const int64_t release_id = CreateDraftRelease();

    // upload the assets
    for( const auto& [filename, data] : m_assets )
        UploadReleaseAsset(release_id, filename, *data);

    // toggle the draft flag, publishing the release
    PublishRelease(release_id);
}


int64_t ReleaseCreator::CreateDraftRelease()
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::tag_name, m_tagName)
                .Write(JK::name, m_releaseTitle)
                .Write(JK::body, SO::ToNewlineLF(GetFormattedReleaseNotes()))
                .Write(JK::draft, true)
                .Write(JK::prerelease, m_prerelease)
                .EndObject();

    const HttpResponse response = m_ghConnection.PostJsonWithAuthentication(
        GitHubConnection::CreateApiUrl("releases"),
        json_writer->ReleaseString()
    );

    if( response.http_status != HttpResponse::Status_201_Created )
        throw CSProException("The draft release could not be created, error: %d", response.http_status);

    const JsonNode json_node = Json::Parse(response.body.ToString());

    // ensure that the upload URL is as expected
    const std::string upload_url = json_node.Get<std::string>(JK::upload_url);

    if( Path::GetFilename(upload_url) != "assets{?name,label}" )
        throw CSProException("Modify this tool to handle upload URLs in the form: " + upload_url);

    return json_node.Get<int64_t>(JK::id);
}


void ReleaseCreator::UploadReleaseAsset(const int64_t release_id, const std::string& filename, const BinaryBlock& data)
{
    const std::string path = FormatText("releases/" Formatter_int64_t "/assets?name=%s",
                                        release_id, Encoders::ToUriComponent(filename).c_str());

    const HttpResponse response = m_ghConnection.PostBinaryWithAuthentication(
        GitHubConnection::CreateUploadUrl(path),
        data
    );

    if( response.http_status != HttpResponse::Status_201_Created )
    {
        throw CSProException("The release asset '%s' could not be uploaded, error: %d",
                             filename.c_str(), response.http_status);
    }
}


void ReleaseCreator::PublishRelease(const int64_t release_id)
{
    const HttpResponse response = m_ghConnection.PatchJsonWithAuthentication(
        GitHubConnection::CreateApiUrl("releases/" + IntToString(release_id)),
        R"({"draft":false})"
    );

    if( response.http_status != HttpResponse::Status_200_OK )
        throw CSProException("The draft release could not published, error: %d", response.http_status);
}
