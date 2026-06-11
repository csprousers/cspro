#include "StdAfx.h"
#include "Builder.h"
#include <zToolsO/Hash.h>


CREATE_JSON_KEY(cspro)
CREATE_JSON_KEY(csweb)
CREATE_JSON_KEY(missing)
CREATE_JSON_KEY(overrides)

// --------------------------------------------------------------------------
// Release
// --------------------------------------------------------------------------

enum class ReleaseAsset { ReleaseNotes, Installer_x86, CSEntry_apk, CSWeb_tarball, CSWeb_zip, WhatsNewHelp };

struct Release
{
    std::string release_date;

    std::string cspro_version;
    int cspro_version_major;
    int cspro_version_minor;
    int cspro_version_patch;

    std::string csweb_version;

    std::map<std::string, std::string> filename_overrides;
    std::vector<std::string> missing_files;

    std::map<std::string, std::string> github_tags; // repository -> tag
};



// --------------------------------------------------------------------------
// ReleaseProcessor
// --------------------------------------------------------------------------

class ReleaseProcessor
{
public:
    ReleaseProcessor(const Inputs& inputs);

    void Process();

private:
    // Reads the release information from the releases.json file.
    void ReadReleases();
    static Release ReadRelease(const JsonNode& json_node);

    // Returns the expected filename of a release asset.
    std::string GetReleaseFilename(const Release& release, ReleaseAsset asset);

    // Returns the expected file path of a release asset.
    std::string GetReleaseFilePath(const Release& release, ReleaseAsset asset);

    // Returns the MD5 and SHA-256 of the file, with these values cached.
    std::tuple<std::string, std::string> GetHashes(const std::string& file_path);

    // Finds the marker and inserts the new HTML into the page.
    static void InsertHtml(std::string& html, std::string_view marker_sv, const std::string& new_html);

    // Creates the HTML for the resource hashes.
    std::string CreateReleaseResourceHashes(const std::string& file_path);

    // Returns the HTML link to a file in the releases directory, potentially adding a mirror link to GitHub.
    static std::string CreateReleasesUrlHtml(const Release& release, const std::string& file_path, const char* mirror_repository);

    // Creates the HTML for the resource links.
    template<ReleaseAsset asset>
    std::string CreateReleaseResourceHtml(const Release& release);

    // Creates the HTML for the links to the source code.
    static std::string CreateReleaseSourceCodeUrls(const Release& release);

    // Creates the HTML table for a release
    std::string CreateReleaseHtml(const Release& release);

private:
    SettingsDb m_settingsDb;
    const Inputs& m_inputs;
    std::vector<Release> m_releases;
};


ReleaseProcessor::ReleaseProcessor(const Inputs& inputs)
    :   m_settingsDb("CSProUsersWebsiteBuilder.db", "file-hashes"),
        m_inputs(inputs)
{
}


void ReleaseProcessor::Process()
{
    ReadReleases();

    // there should be at least one "latest" and one "archived" release
    if( m_releases.size() < 2 )
        throw ProgrammingErrorException();

    const std::string downloads_file_path = Path::Combine(m_inputs.csprousers_input, "downloads", "index.html");
    const std::string releases_file_path = Path::Combine(m_inputs.csprousers_input, "releases", "index.html");

    std::string downloads_html = FileIO::ReadText(downloads_file_path);
    std::string releases_html = FileIO::ReadText(releases_file_path);

    // update the latest release in both the downloads page and the releases page
    constexpr std::string_view LatestReleaseMarker_sv = "{% comment %}latest-release{% endcomment %}";

    const std::string latest_release_html = CreateReleaseHtml(m_releases.front());

    InsertHtml(downloads_html, LatestReleaseMarker_sv, latest_release_html);
    InsertHtml(releases_html, LatestReleaseMarker_sv, latest_release_html);

    // update the archived releases
    constexpr std::string_view ArchivedReleasesMarker_sv = "{% comment %}archived-releases{% endcomment %}";
    constexpr std::string_view ArchivedReleasesSeparator_sv = "\n<br>";

    std::string archived_releases_html;

    for( auto release_itr = m_releases.cbegin() + 1; release_itr != m_releases.cend(); ++release_itr )
    {
        archived_releases_html.append(CreateReleaseHtml(*release_itr))
                              .append(ArchivedReleasesSeparator_sv);
    }

    archived_releases_html.resize(archived_releases_html.size() - ArchivedReleasesSeparator_sv.size());

    InsertHtml(releases_html, ArchivedReleasesMarker_sv, archived_releases_html);

    // save the modified files
    FileIO::WriteText(downloads_file_path, downloads_html, false);
    FileIO::WriteText(releases_file_path, releases_html, false);
}


void ReleaseProcessor::ReadReleases()
{
    const std::string releases_json_file_path = Path::Combine(m_inputs.csprousers_input, "releases", "releases.json");
    const JsonNode json_node = Json::ParseFile(releases_json_file_path);

    for( const JsonNode& release_json_node : json_node.GetArray() )
        m_releases.emplace_back(ReadRelease(release_json_node));

    // sort the releases in order of newest to oldest
    std::sort(m_releases.begin(), m_releases.end(),
        [&](const Release& r1, const Release& r2) { return ( r1.release_date > r2.release_date  ); }
    );
}


Release ReleaseProcessor::ReadRelease(const JsonNode& json_node)
{
    std::string cspro_version = json_node.Get<std::string>(JK::cspro);
    const std::vector<std::string> version = SO::SplitString(cspro_version, '.');

    if( version.size() != 3 )
        throw ProgrammingErrorException();

    std::map<std::string, std::string> filename_overrides;

    json_node.GetOrEmpty(JK::overrides).ForeachNode(
        [&](const std::string_view type_sv, const JsonNode& override_json_node)
        {
            filename_overrides.try_emplace(
                std::string(type_sv),
                override_json_node.Get<std::string>()
            );
        });

    Release release
    {
        json_node.Get<std::string>(JK::date),
        std::move(cspro_version),
        std::stoi(version[0]),
        std::stoi(version[1]),
        std::stoi(version[2]),
        json_node.GetOrConstruct<std::string>(JK::csweb),
        std::move(filename_overrides),
        json_node.GetArrayOrEmpty(JK::missing).GetVector<std::string>()
    };

    // add the GitHub tags
    if( release.cspro_version_major > 8 || release.cspro_version >= "8.0.1" )
    {
        auto set_github_tag = [&](std::string repository)
        {
            std::string tag = FormatText(
                "v%d.%d.%d-%s",
                release.cspro_version_major,
                release.cspro_version_minor,
                release.cspro_version_patch,
                release.release_date.c_str()
            );

            // the override will be prefixed with the repository
            const auto& override_lookup = release.filename_overrides.find(
                FormatText("%s:%s", repository.c_str(), tag.c_str())
            );

            if( override_lookup != release.filename_overrides.cend() )
                tag = override_lookup->second;

            release.github_tags.try_emplace(std::move(repository), std::move(tag));
        };

        set_github_tag("cspro");
        set_github_tag("csweb");
    }

    return release;
}


std::string ReleaseProcessor::GetReleaseFilename(const Release& release, const ReleaseAsset asset)
{
    std::string filename;

    switch( asset )
    {
        case ReleaseAsset::ReleaseNotes:
        {
            filename = SO::Concatenate("cspro-", release.cspro_version, "-release-notes.txt");
            break;
        }

        case ReleaseAsset::Installer_x86:
        {
            filename = SO::Concatenate("cspro-", release.cspro_version, "-windows-x86.exe");
            break;
        }

        case ReleaseAsset::CSEntry_apk:
        {
            if( release.cspro_version_major > 7 || release.cspro_version >= "7.2.1" )
                filename = SO::Concatenate("csentry-", release.cspro_version, ".apk");

            break;
        }

        case ReleaseAsset::CSWeb_tarball:
        {
            if( release.cspro_version_major > 7 || release.csweb_version >= "7.3" )
                filename = SO::Concatenate("csweb-", release.csweb_version, ".tar.gz");

            break;
        }

        case ReleaseAsset::CSWeb_zip:
        {
            if( !release.csweb_version.empty() )
                filename = SO::Concatenate("csweb-", release.csweb_version, ".zip");

            break;
        }

        case ReleaseAsset::WhatsNewHelp:
        {
            if( release.cspro_version_major >= 7 )
                filename = FormatText("what_is_new_in_cspro_%d_%d.html", release.cspro_version_major, release.cspro_version_minor);

            break;
        }

        default:
        {
            throw ProgrammingErrorException();
        }
    }

    // see if the file is missing
    const auto& missing_lookup = std::find(release.missing_files.cbegin(), release.missing_files.cend(), filename);

    if( missing_lookup != release.missing_files.cend() )
    {
        filename.clear();
    }

    // see if the name is overridden
    else
    {
        const auto& override_lookup = release.filename_overrides.find(filename);

        if( override_lookup != release.filename_overrides.cend() )
            filename = override_lookup->second;
    }

    return filename;
}


std::string ReleaseProcessor::GetReleaseFilePath(const Release& release, const ReleaseAsset asset)
{
    std::string filename = GetReleaseFilename(release, asset);

    if( filename.empty() )
        return filename;

    switch( asset )
    {
        case ReleaseAsset::ReleaseNotes:
        case ReleaseAsset::Installer_x86:
        case ReleaseAsset::CSWeb_tarball:
        case ReleaseAsset::CSWeb_zip:
        {
            return Path::Combine(
                m_inputs.csprousers_output,
                "releases",
                FormatText("%d.%d", release.cspro_version_major, release.cspro_version_minor),
                filename
            );
        }

        case ReleaseAsset::CSEntry_apk:
        {
            return Path::Combine(m_inputs.csprousers_output, "apk", filename);
        }

        case ReleaseAsset::WhatsNewHelp:
        {
            return filename;
        }

        default:
        {
            throw ProgrammingErrorException();
        }
    }
}


std::tuple<std::string, std::string> ReleaseProcessor::GetHashes(const std::string& file_path)
{
    auto get_hash = [&](const bool md5)
    {
        const std::string settings_key = FormatText(
            "%s-%d-%s",
            file_path.c_str(),
            static_cast<int>(PortableFunctions::FileModifiedTime(file_path)),
            md5 ? "md5" : "sha-256"
        );

        std::optional<std::string> hash = m_settingsDb.Read<std::string>(settings_key);

        if( !hash.has_value() )
        {
            hash = md5 ? Hash::Md5::CreateFromFile(file_path, true) :
                         Hash::Sha256::CreateFromFile(file_path, true);

            m_settingsDb.Write(settings_key, *hash);
        }

        return std::move(*hash);
    };

    return { get_hash(true), get_hash(false) };
}


void ReleaseProcessor::InsertHtml(std::string& html, const std::string_view marker_sv, const std::string& new_html)
{
    const size_t marker1_start_pos = html.find(marker_sv);
    const size_t marker1_end_pos = marker1_start_pos + marker_sv.length();

    const size_t marker2_start_pos = ( marker1_start_pos != std::string::npos )
        ? html.find(marker_sv, marker1_end_pos)
        : std::string::npos;

    if( marker2_start_pos == std::string::npos )
        throw CSProException(SO::Concatenate("Could not find: ", marker_sv));

    html.erase(marker1_end_pos, marker2_start_pos - marker1_end_pos);

    html.insert(marker1_end_pos, new_html);
}


std::string ReleaseProcessor::CreateReleaseResourceHashes(const std::string& file_path)
{
    const auto [md5, sha256] = GetHashes(file_path);

    auto get_span = [](const char* const type, const std::string& hash)
    {
        return FormatText("<span class=\"release-hash\">%s: %s</span>", type, hash.c_str());
    };

    return get_span("SHA-256", sha256).append(get_span("MD5", md5));
}


std::string ReleaseProcessor::CreateReleasesUrlHtml(const Release& release, const std::string& file_path, const char* const mirror_repository)
{
    const std::string filename = Path::GetFilename(file_path);
    ASSERT(!filename.empty());

    std::string html = FormatText(
        "<a href=\"{{ site.baseurl }}/releases/%d.%d/%s\">%s</a>",
        release.cspro_version_major,
        release.cspro_version_minor,
        filename.c_str(),
        filename.c_str()
    );

    if( mirror_repository != nullptr )
    {
        const auto& lookup = release.github_tags.find(mirror_repository);

        if( lookup != release.github_tags.cend() )
        {
            html.append(FormatText(
                " (<a href=\"https://github.com/csprousers/%s/releases/download/%s/%s\">mirror</a>)",
                lookup->first.c_str(),
                lookup->second.c_str(),
                filename.c_str()
            ));
        }
    }

    return html;
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::Installer_x86>(const Release& release)
{
    const std::string& exe_file_path = GetReleaseFilePath(release, ReleaseAsset::Installer_x86);

    if( exe_file_path.empty() )
        return std::string();

    return CreateReleasesUrlHtml(release, exe_file_path, "cspro") +
           CreateReleaseResourceHashes(exe_file_path);
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::CSEntry_apk>(const Release& release)
{
    const std::string& apk_file_path = GetReleaseFilePath(release, ReleaseAsset::CSEntry_apk);

    if( apk_file_path.empty() )
        return std::string();

    std::string html = FormatText(
        "<a "
        "class=\"apk-link\" "
        "href=\"{{ site.csentry_google_play_url }}\" "
        "data-url=\"{{ site.baseurl }}/apk/%s\">"
        "CSEntry on Google Play"
        "</a>"
        "<span class=\"apk-hashes\" style=\"display: none;\">%s</span>",
        Path::GetFilename(apk_file_path).c_str(),
        CreateReleaseResourceHashes(apk_file_path).c_str()
    );

    return html;
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::CSWeb_zip>(const Release& release)
{
    const std::string zip_file_path = GetReleaseFilePath(release, ReleaseAsset::CSWeb_zip);
    const std::string tarball_file_path = GetReleaseFilePath(release, ReleaseAsset::CSWeb_tarball);

    if( zip_file_path.empty() )
    {
        ASSERT(tarball_file_path.empty());
        return std::string();
    }

    std::string html = CreateReleasesUrlHtml(release, zip_file_path, "csweb") +
                       CreateReleaseResourceHashes(zip_file_path);

    if( !tarball_file_path.empty() )
    {
        html.append("<br>")
            .append(CreateReleasesUrlHtml(release, tarball_file_path, "csweb"))
            .append(CreateReleaseResourceHashes(tarball_file_path));
    }

    return html;
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::ReleaseNotes>(const Release& release)
{
    const std::string txt_file_path = GetReleaseFilePath(release, ReleaseAsset::ReleaseNotes);
    ASSERT(PortableFunctions::FileIsRegular(txt_file_path));

    return CreateReleasesUrlHtml(release, txt_file_path, nullptr);
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::WhatsNewHelp>(const Release& release)
{
    const std::string& html_filename = GetReleaseFilePath(release, ReleaseAsset::WhatsNewHelp);

    if( html_filename.empty() )
        return std::string();

    return FormatText(
        "<a href=\"{{ site.baseurl }}/help/CSPro/%s\">%s</a>",
        html_filename.c_str(),
        html_filename.c_str()
    );
}


std::string ReleaseProcessor::CreateReleaseSourceCodeUrls(const Release& release)
{
    std::string html;

    if( release.github_tags.empty() )
        return html;

    for( const char* const displayable_repository : { "CSPro", "CSWeb" } )
    {
        const auto& lookup = release.github_tags.find(SO::ToLower(displayable_repository));

        if( lookup != release.github_tags.cend() )
        {
            const std::string url = FormatText(
                "<a href=\"https://github.com/csprousers/%s/tree/%s\">%s</a>",
                lookup->first.c_str(),
                lookup->second.c_str(),
                displayable_repository
            );

            SO::AppendWithSeparator(html, url, " &mdash; ");
        }
    }

    return html;
}


std::string ReleaseProcessor::CreateReleaseHtml(const Release& release)
{
    std::string html = FormatText(
        "\n<table class=\"release-table\">"
        "\n<thead><tr><th>CSPro %s &mdash; %s</th><th>Resource</th></tr></thead>"
        "\n<tbody>",
        release.cspro_version.c_str(),
        release.release_date.c_str()
    );

    auto add_row = [&](const cs::string_sz product, const std::string& resource_html)
    {
        if( resource_html.empty() )
            return;

        html.append("\n<tr><td>")
            .append(product.c_str())
            .append("</td><td>")
            .append(resource_html)
            .append("</td></tr>");
    };

    add_row("CSPro Windows Installer (x86)", CreateReleaseResourceHtml<ReleaseAsset::Installer_x86>(release));

    add_row("CSEntry Android Application", CreateReleaseResourceHtml<ReleaseAsset::CSEntry_apk>(release));

    add_row("CSWeb (Apache/IIS Code)", CreateReleaseResourceHtml<ReleaseAsset::CSWeb_zip>(release));

    add_row("Release Notes", CreateReleaseResourceHtml<ReleaseAsset::ReleaseNotes>(release));

    const std::string whats_new_text = FormatText("What's New in CSPro %d.%d", release.cspro_version_major, release.cspro_version_minor);
    add_row(whats_new_text, CreateReleaseResourceHtml<ReleaseAsset::WhatsNewHelp>(release));

    add_row("Source Code", CreateReleaseSourceCodeUrls(release));

    html.append(
        "\n</tbody>"
        "\n</table>"
        "\n"
    );

    return html;
}



// --------------------------------------------------------------------------
// Builder
// --------------------------------------------------------------------------

void Builder::UpdateReleases()
{
    ReleaseProcessor processor(m_inputs);
    processor.Process();
}
