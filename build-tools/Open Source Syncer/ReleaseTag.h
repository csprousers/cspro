#pragma once


struct ReleaseTag
{
    std::string tag_name;    // the display name
    std::string tag_message; // the annotated message
    std::string version;     // the release version
    bool prerelease;         // true if the tag ends in -alpha, -beta, or -rc
    GitCommit commit;        // the commit

    // Populates a list of tags that are named as a "release tag":
    // i.e., v[version].[version].[version][optional text]
    // If combine_releases is true, only the latest tag for each version is included.
    static std::vector<ReleaseTag> Populate(GitRepository& repo, std::string_view first_version_sv,
                                            bool combine_releases);
};
