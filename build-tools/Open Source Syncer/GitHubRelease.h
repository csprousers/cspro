#pragma once


struct GitHubRelease
{
    std::string html_url;
    std::string tag_name;
    std::string name;
    std::string release_notes;
    bool draft;
    bool prerelease;
    size_t assets_count;

    const std::string& GetReleaseName() const;
    std::wstring GetStatus() const;

    static GitHubRelease CreateFromJson(const JsonNode& json_node);
};
