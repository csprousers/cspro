#include "StdAfx.h"
#include "UpdateVersionDlg.h"
#include "resource.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/Versioning.h>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(UpdateVersionDlg, ResizableDlg)
    ON_MESSAGE(WM_APP, OnReadVersionNumbers)
END_MESSAGE_MAP()


UpdateVersionDlg::UpdateVersionDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_UPDATE_VERSION, pParent),
        m_version(Versioning::NumberDetailedText)
{
    SerializeDialogSize("UpdateVersionDlg");
}



void UpdateVersionDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_VERSION, m_version, true);
    DDX_Control(pDX, IDC_LOG, m_log);
}


BOOL UpdateVersionDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    PostMessage(WM_APP);

    return TRUE;
}


LRESULT UpdateVersionDlg::OnReadVersionNumbers(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_log.Clear();

    try
    {
        const CWaitCursor wait_cursor;

        m_versionedFiles = GetVersionedFiles();

        for( auto versioned_file_itr = m_versionedFiles.begin();
             versioned_file_itr != m_versionedFiles.end();  )
        {
            if( !versioned_file_itr->longest_read_version.empty() )
            {
                m_log.AddText("Read '%s' in: %s",
                              versioned_file_itr->longest_read_version.c_str(),
                              versioned_file_itr->file_path.c_str());

                ++versioned_file_itr;
            }

            else
            {
                m_log.AddText(u8"⚠ Could not find version numbers in: "+ versioned_file_itr->file_path);
                versioned_file_itr = m_versionedFiles.erase(versioned_file_itr);
            }
        }
    }

    catch( const std::exception& exception )
    {
        ErrorMessage::Display(exception);
        PostQuitMessage(0);
    }

    return 1;
}


void UpdateVersionDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        const auto [version_numbers, separator] = SplitVersionText(m_version);

        if( version_numbers.size() != 3 )
            throw CSProException("Specify the version in the form: x.x.x");

        size_t files_changed = 0;

        for( const VersionedFile& versioned_file : m_versionedFiles )
        {
            SaveUpdatedVersionedFile(versioned_file, version_numbers);
            m_log.AddText("Modified version numbers in: " + versioned_file.file_path);
            ++files_changed;
        }

        const int result = AfxMessageBox(
            FormatText("Successfully updated %zu files. Close the tool?", files_changed),
            MB_YESNO | MB_DEFBUTTON1
        );

        if( result == IDYES )
        {
            __super::OnOK();
        }

        else
        {
            PostMessage(WM_APP);
        }
    }

    catch( const std::exception& exception )
    {
        ErrorMessage::Display(exception);
    }
}


std::vector<VersionedFile> UpdateVersionDlg::GetVersionedFiles()
{
    const std::string cspro_solution_directory = MakeFullPath(
        PortableFunctions::PathGetDirectory(__FILE__),
        "..\\..\\cspro"
    );

    DirectoryLister directory_lister(true);
    directory_lister.SetNameFilter(std::regex(R"(^(.*\.rc|AssemblyInfo\.cpp|AssemblyInfo.cs)$)"));

    std::vector<VersionedFile> versioned_files;

    for( const std::string& file_path: directory_lister.GetPaths(cspro_solution_directory) )
    {
        try
        {
            versioned_files.emplace_back(ParseVersionedFile(file_path));
        }

        catch( const std::exception& exception )
        {
            throw CSProException("Error processing: %s\n\n%s", file_path.c_str(), exception.what());
        }
    }

    return versioned_files;
}


VersionedFile UpdateVersionDlg::ParseVersionedFile(const std::string& file_path)
{
    BinaryBlock file_content = FileIO::ReadBinary(file_path);
    std::string_view file_content_sv = file_content.as<std::string_view>();
    const TextEncoding text_encoding(file_content_sv, TextEncoding::Type::Utf8);

    if( text_encoding.GetType() == TextEncoding::Type::Utf8Bom )
    {
        file_content_sv.remove_prefix(text_encoding.GetBomLength());
    }

    else if( text_encoding.GetType() != TextEncoding::Type::Utf8 )
    {
        throw CSProException("Only UTF-8 files are supported: " + file_path);
    }

    VersionedFile versioned_file =
    {
        file_path,
        text_encoding.GetType(),
        std::move(file_content)
    };

    const std::string extension = Path::GetExtension(file_path);

    if( SO::EqualsNoCase(extension, "rc") )
    {
        ParseResourceFile(versioned_file, file_content_sv);
    }

    else
    {
        ASSERT(SO::EqualsOneOfNoCase(extension, "cpp", "cs"));
        ParseAssemblyInfo(versioned_file, file_content_sv);
    }

    return versioned_file;
}


std::tuple<std::vector<int>, char> UpdateVersionDlg::SplitVersionText(const std::string& version_text)
{
    const size_t separator_pos = version_text.find_first_of(".,");

    if( separator_pos == std::string::npos )
        throw CSProException("Invalid version separator: " + version_text);

    const char separator = version_text[separator_pos];

    std::vector<int> version_numbers;

    SO::ForeachSection(version_text, separator,
        [&](const std::string& version_sv)
        {
            version_numbers.emplace_back(std::stoi(version_sv));
        });

    return std::make_tuple(std::move(version_numbers), separator);
}


VersionType UpdateVersionDlg::ParseVersionText(const std::string& version_text)
{
    const auto [version_numbers, separator] = SplitVersionText(version_text);

    return ( separator == '.' && version_numbers.size() == 2 ) ? VersionType::TwoWithDot :
           ( separator == '.' && version_numbers.size() == 4 ) ? VersionType::FourWithDot :
           ( separator == ',' && version_numbers.size() == 4 ) ? VersionType::FourWithComma :
           throw CSProException("Unknown version type: " + version_text);
}


void UpdateVersionDlg::ParseFile(VersionedFile& versioned_file, const std::string_view file_content_sv,
                                 const std::regex& regex1, const std::regex* const regex2)
{
    std::smatch matches;

    SO::ForeachLine<std::string>(file_content_sv, true,
        [&](std::string line)
        {
            VersionLine& version_line = versioned_file.version_lines.emplace_back();

            if( std::regex_match(line, matches, regex1) ||
                ( regex2 != nullptr && std::regex_match(line, matches, *regex2) ) )
            {
                if( matches.size() != 4 )
                    throw CSProException("Invalid version line: " + line);

                std::string line_prefix = matches[1].str();

                // only process non-commented lines
                if( line_prefix.find("//") == std::string::npos )
                {
                    std::string read_version = matches[2].str();
                    const VersionType version_type = ParseVersionText(read_version);

                    SO::Replace(read_version, ',', '.');

                    if( !read_version._Starts_with(versioned_file.longest_read_version) &&
                        !versioned_file.longest_read_version._Starts_with(read_version) )
                    {
                        throw CSProException(
                            "There are conflicting version numbers: '%s' + '%s'",
                            versioned_file.longest_read_version.c_str(),
                            read_version.c_str()
                        );
                    }

                    if( versioned_file.longest_read_version.size() < read_version.size() )
                        versioned_file.longest_read_version = std::move(read_version);

                    version_line.emplace_back(std::move(line_prefix));
                    version_line.emplace_back(version_type);
                    version_line.emplace_back(matches[3].str());
                }
            }

            if( version_line.empty() )
                version_line.emplace_back(std::move(line));

            return true;
        });
}


void UpdateVersionDlg::ParseResourceFile(VersionedFile& versioned_file, const std::string_view file_content_sv)
{
    // match lines such as:
    // FILEVERSION 8,1,0,0
    const std::regex regex1(R"(^(.*VERSION )(\d+,\d+,\d+,\d+)(.*)$)");

    // match lines such as
    // VALUE "FileVersion", "8.1.0.0"
    // VALUE "ProductName", "CSPro 8.1"
    const std::regex regex2(R"(^(.*VALUE \".*(?:Version|Name).*\"(?:|CSPro ))((?:\d+\.\d+)(?:\.\d+\.\d+)?)(\".*)$)");

    ParseFile(versioned_file, file_content_sv, regex1, &regex2);
}


void UpdateVersionDlg::ParseAssemblyInfo(VersionedFile& versioned_file, const std::string_view file_content_sv)
{
    // match lines such as:
    // [assembly: AssemblyProduct("CSPro 8.1")]
    // [assembly: AssemblyVersion("8.1.0")]
    const std::regex regex(R"(^(.*\[assembly:.*(?:Product|Version).*\"(?:|CSPro ))(\d.*)(\"\)\].*)$)");

    ParseFile(versioned_file, file_content_sv, regex, nullptr);
}


void UpdateVersionDlg::SaveUpdatedVersionedFile(const VersionedFile& versioned_file, const std::vector<int>& version_numbers)
{
    ASSERT(version_numbers.size() == 3);

    constexpr std::string_view Newline_sv = SO::Newline_crlf_sv;
    std::string updated_file_content;

    for( const VersionLine& version_line : versioned_file.version_lines )
    {
        for( const VersionLineComponent& component : version_line )
        {
            if( std::holds_alternative<std::string>(component) )
            {
                updated_file_content.append(std::get<std::string>(component));
            }

            else
            {
                auto get_formatter = [&]() -> const char*
                {
                    switch( std::get<VersionType>(component) )
                    {
                        case VersionType::TwoWithDot:    return "%d.%d";
                        case VersionType::FourWithDot:   return "%d.%d.%d.0";
                        case VersionType::FourWithComma: return "%d,%d,%d,0";
                        default:                         throw ProgrammingErrorException();
                    }
                };

                updated_file_content.append(FormatText(
                    get_formatter(),
                    version_numbers[0],
                    version_numbers[1],
                    version_numbers[2])
                );
            }
        }

        updated_file_content.append(Newline_sv);
    }

    // trim the last added newline
    updated_file_content.resize(updated_file_content.size() - Newline_sv.length());

    FileIO::WriteText(
        versioned_file.file_path,
        updated_file_content,
        ( versioned_file.text_encoding_type == TextEncoding::Type::Utf8Bom )
    );
}
