#include "StdAfx.h"
#include "Builder.h"
#include <zToolsO/Hash.h>


CREATE_JSON_KEY(csweb)
CREATE_JSON_KEY(missing)
CREATE_JSON_KEY(overrides)
CREATE_JSON_KEY(prerelease)


// --------------------------------------------------------------------------
// Release
// --------------------------------------------------------------------------

enum class ReleaseAsset { ReleaseNotes, Installer_x86, CSEntry_apk, CSWeb_tarball, CSWeb_zip, WhatsNewHelp };

struct Release
{
    std::string release_date;

    std::string prerelease_type;

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
    std::vector<Release> ReadReleases();

    // Reads the release information from the beta.json file.
    std::optional<Release> ReadBeta(const std::string& latest_release_date);

    // Creates a Release object from the JSON.
    static Release ParseRelease(const JsonNode& json_node);

    // Sets GitHub tags for the release.
    static void AddGitHubTags(Release& release);

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

    // Creates the HTML for a beta release.
    std::string CreateBetaHtml(const std::optional<Release>& beta_release);

    // Creates the HTML table for a release.
    std::string CreateReleaseHtml(const Release& release);

    // Creates the HTML listing downloadable APKs.
    std::string CreateApkHtml();

private:
    SettingsDb m_settingsDb;
    const Inputs& m_inputs;
};


ReleaseProcessor::ReleaseProcessor(const Inputs& inputs)
    :   m_settingsDb("CSProUsersWebsiteBuilder.db", "file-hashes"),
        m_inputs(inputs)
{
}


void ReleaseProcessor::Process()
{
    // read the releases
    const std::vector<Release> releases = ReadReleases();

    // there should be at least one "latest" and one "archived" release
    if( releases.size() < 2 )
        throw ProgrammingErrorException();

    const Release& latest_release = releases.front();

    // read an optional beta
    const std::optional<Release> beta_release = ReadBeta(latest_release.release_date);
    ASSERT(!beta_release.has_value() || beta_release->release_date > latest_release.release_date);

    const std::string apk_file_path = Path::Combine(m_inputs.csprousers_input, "apk", "index.html");
    const std::string beta_file_path = Path::Combine(m_inputs.csprousers_input, "beta", "index.html");
    const std::string downloads_file_path = Path::Combine(m_inputs.csprousers_input, "downloads", "index.html");
    const std::string releases_file_path = Path::Combine(m_inputs.csprousers_input, "releases", "index.html");

    std::string apk_html = FileIO::ReadText(apk_file_path);
    std::string beta_html = FileIO::ReadText(beta_file_path);
    std::string downloads_html = FileIO::ReadText(downloads_file_path);
    std::string releases_html = FileIO::ReadText(releases_file_path);

    // update the beta
    constexpr std::string_view BetaMarker_sv = "{% comment %}beta{% endcomment %}";
    InsertHtml(beta_html, BetaMarker_sv, CreateBetaHtml(beta_release));

    // update the latest release in both the downloads page and the releases page
    constexpr std::string_view LatestReleaseMarker_sv = "{% comment %}latest-release{% endcomment %}";
    const std::string latest_release_html = CreateReleaseHtml(latest_release);

    InsertHtml(downloads_html, LatestReleaseMarker_sv, latest_release_html);
    InsertHtml(releases_html, LatestReleaseMarker_sv, latest_release_html);

    // update the archived releases
    constexpr std::string_view ArchivedReleasesMarker_sv = "{% comment %}archived-releases{% endcomment %}";
    constexpr std::string_view ArchivedReleasesSeparator_sv = "\n<br>";

    std::string archived_releases_html;

    for( auto release_itr = releases.cbegin() + 1; release_itr != releases.cend(); ++release_itr )
    {
        archived_releases_html.append(CreateReleaseHtml(*release_itr))
                              .append(ArchivedReleasesSeparator_sv);
    }

    archived_releases_html.resize(archived_releases_html.size() - ArchivedReleasesSeparator_sv.size());

    InsertHtml(releases_html, ArchivedReleasesMarker_sv, archived_releases_html);

    // update the APKs
    constexpr std::string_view ApksMarker_sv = "{% comment %}apks{% endcomment %}";
    InsertHtml(apk_html, ApksMarker_sv, CreateApkHtml());

    // save the modified files
    FileIO::WriteText(apk_file_path, apk_html, false);
    FileIO::WriteText(beta_file_path, beta_html, false);
    FileIO::WriteText(downloads_file_path, downloads_html, false);
    FileIO::WriteText(releases_file_path, releases_html, false);
}


std::vector<Release> ReleaseProcessor::ReadReleases()
{
    const std::string releases_json_file_path = Path::Combine(m_inputs.csprousers_input, "releases", "releases.json");
    const JsonNode json_node = Json::ParseFile(releases_json_file_path);

    std::vector<Release> releases;

    for( const JsonNode& release_json_node : json_node.GetArray() )
    {
        releases.emplace_back(ParseRelease(release_json_node));
        ASSERT(releases.back().prerelease_type.empty());
    }

    // sort the releases in order of newest to oldest
    std::sort(releases.begin(), releases.end(),
        [&](const Release& r1, const Release& r2) { return ( r1.release_date > r2.release_date  ); }
    );

    return releases;
}


std::optional<Release> ReleaseProcessor::ReadBeta(const std::string& latest_release_date)
{
    const std::string beta_json_file_path = Path::Combine(m_inputs.csprousers_input, "beta", "beta.json");
    const JsonNode json_node = Json::ParseFile(beta_json_file_path);

    Release beta_release = ParseRelease(json_node);
    ASSERT(!beta_release.prerelease_type.empty());

    // a beta must be newer than all releases
    if( beta_release.release_date > latest_release_date )
        return beta_release;

    return std::nullopt;
}


Release ReleaseProcessor::ParseRelease(const JsonNode& json_node)
{
    std::string cspro_version = json_node.Get<std::string>(JK::version);
    const std::vector<std::string> version = SO::SplitString(cspro_version, '.');

    if( version.size() != 3 )
        throw ProgrammingErrorException();

    const int cspro_version_major = std::stoi(version[0]);

    std::string csweb_version = ( cspro_version_major >= 7 )
        ? json_node.GetOrDefault(JK::csweb, cspro_version)
        : std::string();

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
        json_node.GetOrConstruct<std::string>(JK::prerelease),
        std::move(cspro_version),
        cspro_version_major,
        std::stoi(version[1]),
        std::stoi(version[2]),
        std::move(csweb_version),
        std::move(filename_overrides),
        json_node.GetArrayOrEmpty(JK::missing).GetVector<std::string>()
    };

    AddGitHubTags(release);

    return release;
}


void ReleaseProcessor::AddGitHubTags(Release& release)
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

        // prerelease tags include the type
        if( !release.prerelease_type.empty() )
            SO::AppendWithSeparator(tag, release.prerelease_type, '-');

        // the override will be be "github-" followed by the repository name
        const auto& override_lookup = release.filename_overrides.find("github-" + repository);

        if( override_lookup != release.filename_overrides.cend() )
            tag = override_lookup->second;

        release.github_tags.try_emplace(std::move(repository), std::move(tag));
    };

    if( release.cspro_version_major > 8 || release.cspro_version >= "8.0.1" )
    {
        set_github_tag("cspro");
        set_github_tag("csweb");
    }

    if( release.cspro_version_major > 7 || release.cspro_version >= "7.5.0" )
    {
        set_github_tag("helps");
        set_github_tag("examples");
    }
}


std::string ReleaseProcessor::GetReleaseFilename(const Release& release, const ReleaseAsset asset)
{
    std::string override_text;
    std::string filename;

    std::string version_text = ( asset == ReleaseAsset::CSWeb_tarball || asset == ReleaseAsset::CSWeb_zip )
        ? release.csweb_version
        : release.cspro_version;

    // preleases include the type and date
    if( !release.prerelease_type.empty() )
    {
        std::string date_without_hyphens = release.release_date;
        SO::Remove(date_without_hyphens, '-');
        SO::AppendWithSeparator(version_text, release.prerelease_type, '-');
        SO::AppendWithSeparator(version_text, date_without_hyphens, '-');
    }

    switch( asset )
    {
        case ReleaseAsset::ReleaseNotes:
        {
            override_text = "release-notes";
            filename = SO::Concatenate("cspro-", version_text, "-release-notes.txt");
            break;
        }

        case ReleaseAsset::Installer_x86:
        {
            override_text = "cspro-x86";
            filename = SO::Concatenate("cspro-", version_text, "-windows-x86.exe");
            break;
        }

        case ReleaseAsset::CSEntry_apk:
        {
            override_text = "csentry";

            if( release.cspro_version_major > 7 || release.cspro_version >= "7.2.1" )
                filename = SO::Concatenate("csentry-", version_text, ".apk");

            break;
        }

        case ReleaseAsset::CSWeb_tarball:
        {
            override_text = "csweb-tarball";

            if( release.cspro_version_major > 7 || release.csweb_version >= "7.3" )
                filename = SO::Concatenate("csweb-", version_text, ".tar.gz");

            break;
        }

        case ReleaseAsset::CSWeb_zip:
        {
            override_text = "csweb-zip";

            if( !release.csweb_version.empty() )
                filename = SO::Concatenate("csweb-", version_text, ".zip");

            break;
        }

        case ReleaseAsset::WhatsNewHelp:
        {
            override_text = "whats-new";

            if( release.cspro_version_major >= 7 )
                filename = FormatText("what_is_new_in_cspro_%d_%d.html", release.cspro_version_major, release.cspro_version_minor);

            break;
        }

        default:
        {
            throw ProgrammingErrorException();
        }
    }

    ASSERT(!override_text.empty());

    // see if the file is missing
    const auto& missing_lookup = std::find(release.missing_files.cbegin(), release.missing_files.cend(), override_text);

    if( missing_lookup != release.missing_files.cend() )
    {
        filename.clear();
    }

    // see if the name is overridden
    else
    {
        const auto& override_lookup = release.filename_overrides.find(override_text);

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
        case ReleaseAsset::CSEntry_apk:
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
    std::string apk_file_path = GetReleaseFilePath(release, ReleaseAsset::CSEntry_apk);

    if( apk_file_path.empty() )
        return std::string();

    std::string html = FormatText(
        "<a "
        "class=\"apk-link\" "
        "href=\"{{ %s }}\" "
        "data-url=\"{{ site.baseurl }}/releases/%d.%d/%s\">"
        "%s"
        "</a>"
        "<span class=\"apk-hashes\" style=\"display: none;\">%s</span>",
        release.prerelease_type.empty() ? "site.csentry_google_play_url" : "page.csentry_google_play_testing_url",
        release.cspro_version_major,
        release.cspro_version_minor,
        Path::GetFilename(apk_file_path).c_str(),
        release.prerelease_type.empty() ? "CSEntry on Google Play" : "<em>Google Play: sign up as a beta tester</em>",
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

    if( !PortableFunctions::FileIsRegular(txt_file_path) )
        throw FileIO::Exception::FileNotFound(txt_file_path);

    return CreateReleasesUrlHtml(release, txt_file_path, nullptr);
}


template<>
std::string ReleaseProcessor::CreateReleaseResourceHtml<ReleaseAsset::WhatsNewHelp>(const Release& release)
{
    const std::string& html_filename = GetReleaseFilePath(release, ReleaseAsset::WhatsNewHelp);

    if( html_filename.empty() )
        return std::string();

    return FormatText(
        "<a href=\"{{ site.baseurl }}/help/CSPro/%s\">What's New in CSPro %d.%d?</a>",
        html_filename.c_str(),
        release.cspro_version_major,
        release.cspro_version_minor
    );
}


std::string ReleaseProcessor::CreateReleaseSourceCodeUrls(const Release& release)
{
    std::string html;

    if( release.github_tags.empty() )
        return html;

    auto add_url = [&](const char* const displayable_repository)
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

            SO::AppendWithSeparator(html, url, " • ");
        }
    };

    add_url("CSPro");
    add_url("CSWeb");

    // omit helps and examples for prereleases
    if( release.prerelease_type.empty() )
    {
        add_url("Helps");
        add_url("Examples");
    }

    return html;
}


std::string ReleaseProcessor::CreateBetaHtml(const std::optional<Release>& beta_release)
{
    if( !beta_release.has_value() )
    {
        return
            "\n<h2>Current Beta</h2>"
            "\n<hr>"
            "\n<p><em>No beta release is currently available. Visit the <a href=\"{{ site.baseurl }}/downloads/\">Downloads</a> page to find the latest release.</em></p>"
            "\n";
    }

    std::string html = FormatText(
        "\n<h2>Current %s</h2>"
        "\n<hr>"
        "\n<p><strong>Prereleases should be used at your own risk!</strong></p>",
        SO::ToProperCase(beta_release->prerelease_type).c_str()
    );

    html.append(CreateReleaseHtml(*beta_release));

    return html;
}


std::string ReleaseProcessor::CreateReleaseHtml(const Release& release)
{
    std::string html = FormatText(
        "\n<table class=\"release-table\">"
        "\n<thead><tr><th>CSPro %s &mdash; %s%s</th><th>Resource</th></tr></thead>"
        "\n<tbody>",
        release.cspro_version.c_str(),
        release.release_date.c_str(),
        release.prerelease_type.empty() ? "" : FormatText(" (%s)", release.prerelease_type.c_str()).c_str()
    );

    auto add_row = [&](const char* const product, const std::string& resource_html)
    {
        if( resource_html.empty() )
            return;

        html.append("\n<tr><td>")
            .append(product)
            .append("</td><td>")
            .append(resource_html)
            .append("</td></tr>");
    };

    add_row("CSPro Windows Installer (x86)", CreateReleaseResourceHtml<ReleaseAsset::Installer_x86>(release));

    add_row("CSEntry Android Application", CreateReleaseResourceHtml<ReleaseAsset::CSEntry_apk>(release));

    add_row("CSWeb (Apache/IIS Code)", CreateReleaseResourceHtml<ReleaseAsset::CSWeb_zip>(release));

    add_row("Release Notes", CreateReleaseResourceHtml<ReleaseAsset::ReleaseNotes>(release));

    add_row("What's New?", CreateReleaseResourceHtml<ReleaseAsset::WhatsNewHelp>(release));

    add_row("Source Code", CreateReleaseSourceCodeUrls(release));

    html.append(
        "\n</tbody>"
        "\n</table>"
        "\n"
    );

    return html;
}


std::string ReleaseProcessor::CreateApkHtml()
{
    const std::string releases_directory = Path::Combine(m_inputs.csprousers_output, "releases");

    // find all APKs and group them by version
    std::map<std::string, std::vector<std::string>> version_to_apk_file_paths;

    DirectoryLister directory_lister(true);
    directory_lister.SetNameFilter("*.apk");

    for( std::string& apk_file_path : directory_lister.GetPaths(releases_directory) )
    {
        const std::string apk_filename = Path::GetFilename(apk_file_path);

        ASSERT(Path::IsSlashChar(apk_file_path[apk_file_path.length() - apk_filename.length() - 1]));
        std::string version = Path::GetFilename(std::string_view(apk_file_path).substr(0, apk_file_path.length() - apk_filename.length() - 1));

        version_to_apk_file_paths[std::move(version)].emplace_back(std::move(apk_file_path));
    }

    // write out all APKs, sorted by version
    std::string html;

    for( auto itr = version_to_apk_file_paths.rbegin(); itr != version_to_apk_file_paths.rend(); ++itr )
    {
        html.append("\n<tr><td>")
            .append(itr->first)
            .append("</td><td>");

        // sort the APKs in order of newest to oldest
        std::vector<std::string>& apk_file_paths = itr->second;

        std::sort(apk_file_paths.begin(), apk_file_paths.end(),
            [&](const std::string& apk1, const std::string& apk2) { return ( apk1 > apk2 ); }
        );

        for( auto apk_itr = apk_file_paths.cbegin(); apk_itr != apk_file_paths.cend(); ++apk_itr )
        {
            const std::string& apk_file_path = *apk_itr;
            const std::string apk_filename = Path::GetFilename(apk_file_path);

            if( apk_itr != apk_file_paths.cbegin() )
                html.append("<br>");

            html.append(FormatText(
                "<a href=\"{{ site.baseurl }}/releases/%s/%s\">%s</a>"
                "<span class=\"apk-hashes\">%s</span>",
                itr->first.c_str(),
                apk_filename.c_str(),
                apk_filename.c_str(),
                CreateReleaseResourceHashes(apk_file_path).c_str()
            ));
        }

        html.append("</td></tr>");
    }

    html.push_back('\n');

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
