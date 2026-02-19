#pragma once

#include "GitHubConnection.h"


class ReleaseCreator
{
public:
    ReleaseCreator(Controller& controller, SharableString tag_name);

    // Getters and setters used by CreateReleaseDlg.
    const SharableString& GetTagName() const { return m_tagName; }

    const SharableString& GetLibrariesTag() const { return m_librariesTag; }

    const SharableString& GetVersionText() const { return m_versionText; }

    void SetReleaseTitle(SharableString title) { m_releaseTitle = std::move(title); }

    bool GetPrerelease() const          { return m_prerelease; }
    void SetPrerelease(bool prerelease) { m_prerelease = prerelease; }

    const std::string& GetReleaseNotes() const { return m_releaseNotes; }
    void SetReleaseNotes(std::string notes)    { m_releaseNotes = std::move(notes); }

    void SetAsset(std::string filename, std::shared_ptr<const BinaryBlock> data);

    // Returns an identifier to use for the filenames for a given release.
    // For example, "-beta-20260209" for a beta or "" for a release.
    std::string GetReleaseFilenameIdentifier();

    // Returns a non-null pointer to the file content in the open source repository,
    // throwing an exception if not found.
    std::shared_ptr<const BinaryBlock> GetFileInOpenSourceRepository(const std::string& os_repo_path);

    // Replaces the fills in the release notes with the values for this release.
    std::string GetFormattedReleaseNotes() const;

    // Creates the release, first as a draft, and then when all assets
    // have been successfully uploaded, the draft is published.
    void CreateRelease();

private:
    // Returns the expected release type based on the tag name.
    enum class ReleaseType { Alpha, Beta, ReleaseCandidate, Release };
    static ReleaseType ParseReleaseType(const std::string& tag_name);

    void ParseVersion();

    int64_t CreateDraftRelease();
    void UploadReleaseAsset(int64_t release_id, const std::string& filename, const BinaryBlock& data);
    void PublishRelease(int64_t release_id);

private:
    Controller& m_controller;
    GitHubConnection m_ghConnection;

    SharableString m_tagName;
    SharableString m_librariesTag;

    std::string m_openSourceCommitSHA;
    std::string m_privateCommitSHA;

    SharableString m_versionText;
    std::tuple<int, int, int> m_version;

    SharableString m_releaseTitle;
    bool m_prerelease;
    std::string m_releaseNotes;

    std::map<std::string, std::shared_ptr<const BinaryBlock>> m_assets;

    std::optional<GitTree> m_openSourceTree;
    std::map<std::string, std::shared_ptr<const BinaryBlock>> m_loadedRepoFiles;
};
