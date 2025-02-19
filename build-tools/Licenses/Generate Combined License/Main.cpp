#define WIN_DESKTOP
#include <engine/StandardSystemIncludes.h>
#include <engine/StrictCompilerErrors.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8.h>
#include <zJson/Json.h>
#include <zUtilO/CSProExecutables.h>
#include <zHtml/HtmlWriter.h>


CREATE_JSON_KEY(projectName)
CREATE_JSON_KEY(projectUrl)
CREATE_JSON_KEY(licenseFilename)
CREATE_JSON_KEY(licenseUrl)


struct License
{
    std::string project_name;
    std::string project_url;
    std::string license_filename;
    std::string license_url;
    std::string license_text;
};


class LicenseGenerator
{
public:
    void Generate();

    size_t GetNumberLicenses() const { return m_licenses.size(); }

private:
    void LoadLicenseDetails();
    std::string LoadLicenseText(const std::string& project_name);

    void CreateLicense();
    std::string PrependFourSpacesToEachLine(std::string text);
    const std::string& GetAnchorId(const std::string& project_name);
    std::string CreateLicenseListHtml();
    std::string CreateLicenseTextHtml(const std::string& license_text);
    std::string CreateLicensesHtml();

private:
    std::string m_inputsDirectory;
    std::string m_licensesDirectory;
    std::vector<License> m_licenses;
    std::string m_csproLicenseText;
    std::map<std::string, std::string> m_anchorIds;
};


int main()
{
    std::string message;
    unsigned int message_flags = MB_OK;

    try
    {
        LicenseGenerator generator;
        generator.Generate();

        message = FormatText("Successfully created a combined license for %d projects.", static_cast<int>(generator.GetNumberLicenses()));
    }

    catch( const CSProException& exception )
    {
        message = exception.what();
        message_flags |= MB_ICONEXCLAMATION;
    }

    MessageBoxW(nullptr, TC::ToWide(message).c_str(), L"Generate Combined License", message_flags);
}


void LicenseGenerator::Generate()
{
    m_inputsDirectory = MakeFullPath(CSProExecutables::GetModuleDirectory(), "..\\..\\");
    m_licensesDirectory = Path::Combine(m_inputsDirectory, "Licenses");

    LoadLicenseDetails();

    for( License& license : m_licenses )
        license.license_text = LoadLicenseText(license.project_name);

    m_csproLicenseText = LoadLicenseText("CSPro");

    CreateLicense();
}


void LicenseGenerator::LoadLicenseDetails()
{
    const std::string details_file_path = Path::Combine(m_inputsDirectory, "Details.json");
    const JsonNode json_node = Json::ParseFile(details_file_path);

    for( const JsonNode& license_node : json_node.GetArray() )
    {
        m_licenses.emplace_back(License
            {
                license_node.Get<std::string>(JK::projectName),
                license_node.GetOrConstruct<std::string>(JK::projectUrl),
                license_node.GetOrConstruct<std::string>(JK::licenseFilename),
                license_node.GetOrConstruct<std::string>(JK::licenseUrl),
            });
    }

    // order the licenses by project name
    std::sort(m_licenses.begin(), m_licenses.end(),
              [](const License& l1, const License& l2) { return ( SO::CompareNoCase(l1.project_name, l2.project_name) < 0 ); });
}


std::string LicenseGenerator::LoadLicenseText(const std::string& project_name)
{
    const std::string license_file_path = PortableFunctions::CreateFilePath(m_licensesDirectory, project_name, ".txt");

    std::string license_text = FileIO::ReadText(license_file_path);
    SO::MakeTrimRight(license_text);

    // standardize the line endings
    SO::MakeNewlineLF(license_text);

    // remove form file characters
    SO::Remove(license_text, '\f');

    return license_text;
}


void LicenseGenerator::CreateLicense()
{
    const std::string license_template_path = Path::Combine(m_inputsDirectory, "Licenses-Template.html");
    const std::string license_output_path = Path::Combine(m_inputsDirectory, "Licenses.html");

    std::string license_html = FileIO::ReadText(license_template_path);

    SO::Replace(license_html, "{{ list-list }}", PrependFourSpacesToEachLine(CreateLicenseListHtml()));
    SO::Replace(license_html, "{{ cspro-license }}", PrependFourSpacesToEachLine(CreateLicenseTextHtml(m_csproLicenseText)));
    SO::Replace(license_html, "{{ licenses }}", PrependFourSpacesToEachLine(CreateLicensesHtml()));

    FileIO::WriteText(license_output_path, license_html, false);
}


std::string LicenseGenerator::PrependFourSpacesToEachLine(std::string text)
{
    ASSERT(!SO::IsWhitespace(text) && text.find('\r') == std::string::npos);

    size_t current_pos = 0;

    while( true )
    {
        const size_t newline_pos = text.find('\n', current_pos);
        const std::string_view this_line_sv = std::string_view(text).substr(current_pos, newline_pos - current_pos);
        size_t chars_added;

        // trim empty lines
        if( SO::IsWhitespace(this_line_sv) )
        {
            text.erase(current_pos, this_line_sv.length());
            chars_added = 0;
        }

        // otherwise insert the spaces
        else
        {
            text.insert(current_pos, SO::SingleTabAsSpaces);
            chars_added = 4;
        }

        if( newline_pos == std::string::npos )
            break;

        current_pos = newline_pos + 1 + chars_added;
    }

    return text;
}


const std::string& LicenseGenerator::GetAnchorId(const std::string& project_name)
{
    const auto& lookup = m_anchorIds.find(project_name);

    if( lookup != m_anchorIds.cend() )
        return lookup->second;

    // only use lowercase letters
    std::string anchor_id;

    for( int ch : project_name )
    {
        ch = std::tolower(ch);

        if( std::islower(ch) )
            anchor_id.push_back(static_cast<char>(ch));
    }

    // ensure nothing is duplicated
    if( std::find_if(m_anchorIds.cbegin(), m_anchorIds.cend(),
                     [&](const auto& key_value) { return ( anchor_id == key_value.second ); }) != m_anchorIds.cend() )
    {
        throw CSProException("Duplicate anchor ID: %s", anchor_id.c_str());
    }

    return m_anchorIds.try_emplace(project_name, std::move(anchor_id)).first->second;
}



std::string LicenseGenerator::CreateLicenseListHtml()
{
    HtmlStringWriter html_writer;

    html_writer << "<ul>\n";

    for( const License& license : m_licenses )
    {
        html_writer << SO::SingleTabAsSpaces << "<li><a href=\"#";
        html_writer.WriteTagValue(GetAnchorId(license.project_name));
        html_writer << "\">" << license.project_name << "</a></li>\n";
    }

    html_writer << "</ul>";

    return html_writer.str();
}


std::string LicenseGenerator::CreateLicenseTextHtml(const std::string& license_text)
{
    constexpr std::string_view LicenseNewline_sv = "<br>\n";

    HtmlStringWriter html_writer;

    html_writer << "<p class=\"license\">\n";

    SO::ForeachLine(license_text, true,
        [&](const std::string_view line_sv)
        {
            html_writer << SO::SingleTabAsSpaces << line_sv;
            html_writer.WriteRaw(LicenseNewline_sv);
        });

    std::string html = html_writer.str();

    // remove the last newline
    const auto& last_newline_pos = html.end() - LicenseNewline_sv.size();
    ASSERT(LicenseNewline_sv == &*last_newline_pos);
    html.erase(last_newline_pos, html.end());

    html.append("\n</p>");

    return html;
}


std::string LicenseGenerator::CreateLicensesHtml()
{
    HtmlStringWriter html_writer;

    for( const License& license : m_licenses )
    {
        html_writer << "<h2 id=\"";
        html_writer.WriteTagValue(GetAnchorId(license.project_name));
        html_writer << "\">" << license.project_name << "</h2>\n\n";

        html_writer << "<p>\n";

        if( !license.project_url.empty() )
        {
            html_writer << SO::SingleTabAsSpaces << "Project website: <a href=\"";
            html_writer.WriteTagValue(license.project_url);
            html_writer << "\">" << license.project_url << "</a><br>\n";
        }

        if( !license.license_filename.empty() || !license.license_url.empty() )
        {
            html_writer << SO::SingleTabAsSpaces << "License ";

            if( !license.license_filename.empty() )
            {
                html_writer << "found in the file " << license.license_filename;

                if( !license.license_url.empty() )
                    html_writer << " and ";
            }

            if( !license.license_url.empty() )
            {
                html_writer << "available at <a href=\"";
                html_writer.WriteTagValue(license.license_url);
                html_writer << "\">" << license.license_url << "</a>";
            }
        }

        html_writer << "\n</p>\n\n";

        html_writer.WriteRaw(CreateLicenseTextHtml(license.license_text));

        html_writer << "\n\n";
    }

    return html_writer.str();
}
