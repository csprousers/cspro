#pragma once


namespace SettingsKeys
{
    constexpr std::string_view OpenSourceCodeDirectory_sv      = "open-source-code-directory";
    constexpr std::string_view OpenSourceLibrariesDirectory_sv = "open-source-libraries-directory";

    constexpr std::string_view GitHubPAT_sv                    = "github-pat";

    constexpr std::string_view FeatureBranchSyncerTarget_sv    = "feature-branch-syncer-branch-name";

    constexpr std::string_view GitHubReleases_sv               = "github-releases";
    constexpr std::string_view GitHubTags_sv                   = "github-tags";

    constexpr std::string_view Libraries_sv                    = "libraries";

    constexpr const char* SqlitePrefix                         = "sqlite-";
}
