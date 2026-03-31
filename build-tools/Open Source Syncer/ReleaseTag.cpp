#include "StdAfx.h"
#include "ReleaseTag.h"


std::vector<ReleaseTag> ReleaseTag::Populate(GitRepository& repo, const std::string_view first_version_sv,
                                             const bool combine_releases)
{
    std::vector<ReleaseTag> release_tags;

    std::regex tag_regex(R"(^refs\/tags\/v(\d+\.\d+\.\d+).*$)");
    std::smatch matches;

    repo.ForeachTag(
        [&](const GitTag tag)
        {
            constexpr bool keep_processing = true;

            if( !std::regex_search(tag.GetName(), matches, tag_regex) )
                return keep_processing;

            std::string version = matches.str(1);

            if( version < first_version_sv )
                return keep_processing;

            std::string tag_name = tag.GetDisplayName();
            std::string tag_message = tag.GetMessage(repo);

            const bool prerelease = ( tag_name.find("-alpha") != std::string::npos ||
                                      tag_name.find("-beta") != std::string::npos ||
                                      tag_name.find("-rc") != std::string::npos );

            GitCommit commit = repo.LookupCommit(tag);

            const auto& lookup =
                !combine_releases ? release_tags.end() :
                                    std::find_if(release_tags.begin(), release_tags.end(),
                                                 [&](const ReleaseTag& rt) { return ( version == rt.version ); });

            if( lookup == release_tags.end() )
            {
                release_tags.emplace_back(
                    ReleaseTag { std::move(tag_name), std::move(tag_message), std::move(version), prerelease, std::move(commit) }
                );
            }

            // when combining releases, associate the tag with the latest commit in case of
            // multiple tags for the same version (e.g., v7.6.1-Apr20 and v7.6.1-Apr26)
            else if( lookup->commit.GetAuthor().GetWhen() < commit.GetAuthor().GetWhen() )
            {
                ASSERT(combine_releases);
                lookup->tag_name = std::move(tag_name);
                lookup->tag_message = std::move(tag_message);
                lookup->prerelease = prerelease;
                lookup->commit = std::move(commit);
            }

            return keep_processing;
        });

    // sort by version
    std::sort(release_tags.begin(), release_tags.end(),
              [&](const ReleaseTag& rt1, const ReleaseTag& rt2) { return ( rt1.tag_name < rt2.tag_name ); });

    // validate the tags
    const size_t prerelease_tags =
        std::count_if(release_tags.cbegin(), release_tags.cend(),
                      [&](const ReleaseTag& rt) { return rt.prerelease; });

    if( prerelease_tags > 1 )
    {
        throw CSProException("There should not be %zu prerelease tags.", prerelease_tags);
    }

    else if( prerelease_tags == 1 &&
             !release_tags.back().prerelease )
    {
        throw CSProException("The only prerelease tag must be the final one: " + release_tags.back().tag_name);
    }

    return release_tags;
}
