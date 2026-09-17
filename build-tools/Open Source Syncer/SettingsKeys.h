#pragma once


namespace SettingsKeys
{
    constexpr std::string_view OpenSourceCodeDirectory_sv           = "open-source-code-directory";
    constexpr std::string_view OpenSourceGitHubReleases_sv          = "open-source-github-releases";
    constexpr std::string_view OpenSourceGitHubTags_sv              = "open-source-github-tags";

    constexpr std::string_view ThirdPartyLibrariesDirectory_sv      = "third-party-libraries-directory";
    constexpr std::string_view ThirdPartyLibrariesGitHubReleases_sv = "third-party-libraries-github-releases";

    constexpr std::string_view GitHubPAT_sv                         = "github-pat";

    constexpr std::string_view FeatureBranchSyncerTarget_sv         = "feature-branch-syncer-branch-name";

    constexpr std::string_view FileHashes_sv                        = "file-hashes";

    constexpr const char* SqlitePrefix                              = "sqlite-";
}
