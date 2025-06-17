#include "StdAfx.h"
#include "Builder.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zJson/Json.h>
#include <zUtilO/TemporaryFile.h>
#include <zZip/ZipFile.h>


Builder::Builder(Directories directories, LoggingListBox& logging_list_box)
    :   m_directories(std::move(directories)),
        m_loggingListBox(logging_list_box)
{
}


void Builder::RecycleDirectory(const std::string& directory)
{
    if( !PortableFunctions::FileIsDirectory(directory) )
        return;

    m_loggingListBox.AddText("Recycling " + directory);

    wchar_t complete_from_path[MAX_PATH];
    const int path_length = GetFullPathName(TC::ToWide(directory).c_str(), MAX_PATH, complete_from_path, nullptr);

    if( path_length == 0 || path_length >= MAX_PATH )
        throw CSProException("GetFullPathName error: %s", directory.c_str());

    SHFILEOPSTRUCT info = { nullptr };
    info.wFunc = FO_DELETE;
    info.fFlags = FOF_NOCONFIRMATION | FOF_ALLOWUNDO;
    info.pFrom = complete_from_path;

    if( SHFileOperation(&info) != 0 )
        throw CSProException("Error recycling: " + directory);
}


void Builder::CopyFile(const std::string& input_file_path, const std::string& output_file_path,
                       const FileOverwriteFlag file_overwrite_flag/* = FileOverwriteFlag::Fail*/, const bool add_message_to_log/* = true*/)
{
    if( add_message_to_log )
        m_loggingListBox.AddText("Copying file:\n    " + input_file_path + "\n    " + output_file_path);

    FileIO::CreateDirectoriesForFile(output_file_path);
    PortableFunctions::FileCopyWithExceptions(input_file_path, output_file_path, file_overwrite_flag);
}


void Builder::CopyDirectoryRecursive(const std::string& input_directory, const std::string& output_directory,
                                     const FileOverwriteFlag file_overwrite_flag/* = FileOverwriteFlag::Fail*/)
{
    m_loggingListBox.AddText("Copying directory:\n    " + input_directory + "\n    " + output_directory);

    DirectoryLister directory_lister(true);

    for( const std::string& input_file_path : directory_lister.GetPaths(input_directory) )
    {
        ASSERT(SO::StartsWithNoCase(input_file_path, input_directory));

        CopyFile(input_file_path,
                 Path::Combine(output_directory, input_file_path.substr(input_directory.length())),
                 file_overwrite_flag,
                 false);
    }
}


void Builder::BuildDocSet(const std::string& csdocset_file_path, const std::variant<const char*, BuildBlog> build_name_or_build_blog)
{
    const std::string csdocument_exe = Path::Combine(m_directories.cspro_root, R"(cspro\debug\bin\CSDocument.exe)");

    if( !PortableFunctions::FileIsRegular(csdocument_exe) )
        throw CSProException("CSDocument must exist at: " + csdocument_exe);

    std::string command = EscapeCommandLineArgument(csdocument_exe);
    const char* evaluated_build_name;

    if( std::holds_alternative<const char*>(build_name_or_build_blog) )
    {
        evaluated_build_name = std::get<const char*>(build_name_or_build_blog);

        command.append(" -build ").append(EscapeCommandLineArgument(evaluated_build_name))
               .append(" -input ").append(EscapeCommandLineArgument(csdocset_file_path));
    }

    else
    {
        evaluated_build_name = "CSPro Users Blog";

        command.append(" -csprousers-blog ")
               .append(" -documentSet ").append(EscapeCommandLineArgument(csdocset_file_path))
               .append(" -output ").append(EscapeCommandLineArgument(std::get<BuildBlog>(build_name_or_build_blog).posts_directory));
    }

    int return_code;

    if( !RunProgram(TC::ToWide(command), &return_code, SW_SHOWNA, true, true) )
        throw CSProException("Error running CSDocument (build '%s'): %s", evaluated_build_name, csdocset_file_path.c_str());
}


void Builder::BuildSite()
{
    m_loggingListBox.AddText("Building the site for production using Jekyll...");

    const std::string ruby_exe = Path::Combine(m_directories.ruby, "bin", "ruby.exe");
    const std::string jekyll_sh = Path::Combine(m_directories.ruby, "bin", "jekyll");

    if( !PortableFunctions::FileIsRegular(ruby_exe) ||
        !PortableFunctions::FileIsRegular(jekyll_sh) )
    {
        throw CSProException("Ruby and Jekyll must exist at:\n%s\n%s", ruby_exe.c_str(), jekyll_sh.c_str());
    }

    const std::string site_output_directory = Path::Combine(m_directories.csprousers_input, "_site");
    RecycleDirectory(site_output_directory);

    const std::string command = EscapeCommandLineArgument(ruby_exe)
                                .append(" ").append(EscapeCommandLineArgument(jekyll_sh))
                                .append(" build --config")
                                .append(" ").append(EscapeCommandLineArgument(Path::Combine(m_directories.csprousers_input, "_config.yml")))
                                .append(",").append(EscapeCommandLineArgument(Path::Combine(m_directories.csprousers_input, "_config_shared.yml")))
                                .append(",").append(EscapeCommandLineArgument(Path::Combine(m_directories.csprousers_input, "_config_production.yml")));

    int return_code;

    if( !RunProgram(TC::ToWide(command), &return_code, SW_SHOWNA, true, true, TC::ToWide(m_directories.csprousers_input).c_str()) ||
        !PortableFunctions::FileIsDirectory(site_output_directory) )
    {
        throw CSProException("Error running Jekyll: " + command);
    }

    ASSERT(return_code == 0);

    CopyDirectoryRecursive(site_output_directory, m_directories.csprousers_output, FileOverwriteFlag::Always);
}


void Builder::UpdateBlog()
{
    m_loggingListBox.AddText("Building the blog...");

    const std::string posts_directory = Path::Combine(m_directories.csprousers_input, "_posts");
    RecycleDirectory(posts_directory);

    const std::string csdocset_file_path = Path::Combine(m_directories.csprousers_input, "blog", "CSPro Users Blog.csdocset");

    m_loggingListBox.AddText(FormatText("Converting the blog posts in %s...", Path::GetFilename(csdocset_file_path).c_str()));
    BuildDocSet(csdocset_file_path, BuildBlog { posts_directory });

    const std::vector<std::string> post_file_paths = DirectoryLister().GetPaths(posts_directory);

    if( !post_file_paths.empty() )
    {
        m_loggingListBox.AddText(FormatText("Successfully converted %d blog post%s, the last one being:\n    %s",
                                            static_cast<int>(post_file_paths.size()), PluralizeWord(post_file_paths.size()),
                                            Path::GetFilename(post_file_paths.back()).c_str()));
    }
}


void Builder::UpdateHelps()
{
    m_loggingListBox.AddText("Building the helps...");

    const std::string helps_output_directory = Path::Combine(m_directories.csprousers_output, "help");
    RecycleDirectory(helps_output_directory);

    // copy the resource files
    const std::string resource_files_json_file_path = Path::Combine(m_directories.helps, "resource-files.json");
    m_loggingListBox.AddText(FormatText("Copying resource files specified in %s...", resource_files_json_file_path.c_str()));

    JsonReaderInterface json_reader_interface(m_directories.helps);
    const JsonNode json_node = Json::ParseFile(resource_files_json_file_path, &json_reader_interface);

    for( const JsonNode& file_json_node : json_node.GetArray() )
    {
        const std::string resource_file_path = file_json_node.GetAbsolutePath();
        CopyFile(resource_file_path,
                 Path::Combine(helps_output_directory, "resources", Path::GetFilename(resource_file_path)));
    }

    const std::string csdocument_outputs_directory = Path::Combine(m_directories.helps, "Outputs");
    RecycleDirectory(csdocument_outputs_directory);

    DirectoryLister directory_lister(true);
    directory_lister.SetNameFilter("*.csdocset");

    for( const std::string& csdocset_file_path : directory_lister.GetPaths(m_directories.helps) )
    {
        // build the website
        m_loggingListBox.AddText(FormatText("Building the website for %s...", Path::GetFilenameWithoutExtension(csdocset_file_path).c_str()));
        BuildDocSet(csdocset_file_path, "CSPro Users Help Website");
    }

    CopyDirectoryRecursive(Path::Combine(csdocument_outputs_directory, "Website"),
                           helps_output_directory);
}


void Builder::UpdateMobileWorkshop()
{
    m_loggingListBox.AddText("Building the mobile workshop materials...");

    const std::string mobile_workshop_output_directory = Path::Combine(m_directories.csprousers_output, "mobile-workshop");
    RecycleDirectory(mobile_workshop_output_directory);

    const std::string csdocset_file_path = Path::Combine(m_directories.mobile_workshop, "CSProMobileWorkshop", "CSProMobileWorkshop.csdocset");

    const std::string csdocument_outputs_directory = Path::Combine(m_directories.mobile_workshop, "Outputs");
    RecycleDirectory(csdocument_outputs_directory);

    // build the website
    m_loggingListBox.AddText("Building the website...");
    BuildDocSet(csdocset_file_path, "CSPro Users Workshop Website");
    CopyDirectoryRecursive(Path::Combine(csdocument_outputs_directory, "mobile-workshop"),
                           mobile_workshop_output_directory);

    // build the PDF
    m_loggingListBox.AddText("Building the PDF...");
    BuildDocSet(csdocset_file_path, "PDF Documentation");
    CopyFile(Path::Combine(csdocument_outputs_directory, "CSProMobileWorkshop.pdf"),
             Path::Combine(mobile_workshop_output_directory, "pdf", "cspro-mobile-workshop.pdf"));

    // create the materials ZIP file
    m_loggingListBox.AddText("Building the materials ZIP file...");
    std::vector<std::string> zip_input_file_paths;

    DirectoryLister directory_lister(true);
    directory_lister.AddPaths(zip_input_file_paths, Path::Combine(m_directories.mobile_workshop, "FilesForExercises"));
    directory_lister.AddPaths(zip_input_file_paths, Path::Combine(m_directories.mobile_workshop, "Questionnaire"));

    std::string zip_output_file_path = Path::Combine(mobile_workshop_output_directory, "materials", "cspro-mobile-workshop-materials.zip");
    FileIO::CreateDirectoriesForFile(zip_output_file_path);

    ZipCreator zip_creator(std::move(zip_output_file_path));
    zip_creator.AddFiles(zip_input_file_paths);
    zip_creator.Close();
}


void Builder::UpdateGooglePlayPrivacyPolicy()
{
    m_loggingListBox.AddText("Creating the Google Play privacy policy...");

    const std::string gcl_exe = Path::Combine(m_directories.cspro_root, R"(build-tools\Licenses\Generate Combined License\Debug\Generate Combined License.exe)");

    if( !PortableFunctions::FileIsRegular(gcl_exe) )
        throw CSProException("The Generate Combined License program must exist at: " + gcl_exe);

    const std::string privacy_path_directory = Path::Combine(m_directories.csprousers_input, "privacy");
    const std::string privacy_path_template_file_path = Path::Combine(privacy_path_directory, "privacy-policy-template.html");
    const std::string privacy_path_output_file_path = Path::Combine(privacy_path_directory, "privacy-policy.html");

    // create the license as a temporary file
    TemporaryFile privacy_path_output_temporary_file;
    const int64_t privacy_path_output_initial_size = PortableFunctions::FileSize(privacy_path_output_temporary_file.GetPath());

    const std::string command = EscapeCommandLineArgument(gcl_exe)
                                .append(" ").append(EscapeCommandLineArgument(privacy_path_template_file_path))
                                .append(" ").append(EscapeCommandLineArgument(privacy_path_output_temporary_file.GetPath()));
    int return_code;

    if( !RunProgram(TC::ToWide(command), &return_code, SW_SHOWNA, true, true) ||
        privacy_path_output_initial_size == PortableFunctions::FileSize(privacy_path_output_temporary_file.GetPath()) )
    {
        throw CSProException("Error running the Generate Combined License program.");
    }

    // copy the license to its destination
    PortableFunctions::FileCopyWithExceptions(privacy_path_output_temporary_file.GetPath(), privacy_path_output_file_path, FileOverwriteFlag::Always);
}
