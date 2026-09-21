#include "StdAfx.h"
#include "GitHubRelease.h"


const std::string& GitHubRelease::GetReleaseName() const
{
    // release names are not available in old releases
    return !name.empty() ? name :
                           tag_name;
}


std::wstring GitHubRelease::GetStatus() const
{
    std::wstring status = prerelease ? L"Prerelease" :
                                       L"Release";

    if( draft )
        status.insert(0, L"(Draft) ");

    return status;
}


GitHubRelease GitHubRelease::CreateFromJson(const JsonNode& json_node)
{
    return GitHubRelease
    {
        json_node.Get<std::string>(JK::html_url),
        json_node.Get<std::string>(JK::tag_name),
        json_node.Get<std::string>(JK::name),
        json_node.Get<std::string>(JK::body),
        json_node.Get<bool>(JK::draft),
        json_node.Get<bool>(JK::prerelease),
        json_node.GetArray(JK::assets).size()
    };
}
