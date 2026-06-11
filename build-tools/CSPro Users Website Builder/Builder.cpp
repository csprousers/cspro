#include "StdAfx.h"
#include "Builder.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/File.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zZip/ZipFile.h>
#include <zGit/GitBlob.h>
#include <zGit/GitBranch.h>
#include <zGit/GitCommit.h>
#include <zGit/GitDiff.h>
#include <zGit/GitTree.h>
#include <external/libgit2/include/git2/diff.h>


namespace
{
    std::string_view HelpDirectory_sv           = "help";
    std::string_view MobileWorkshopDirectory_sv = "mobile-workshop";
}


Builder::Builder(Inputs inputs, LoggingListBox& logging_list_box)
    :   m_inputs(std::move(inputs)),
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
                       const bool add_message_to_log/* = true*/)
{
    if( add_message_to_log )
        m_loggingListBox.AddText("Copying file:\n    " + input_file_path + "\n    " + output_file_path);

    FileIO::CreateDirectoriesForFile(output_file_path);
    PortableFunctions::FileCopyWithExceptions(input_file_path, output_file_path, FileOverwriteFlag::Always);
}


void Builder::CopyDirectoryRecursive(const std::string& input_directory, const std::string& output_directory)
{
    m_loggingListBox.AddText("Copying directory:\n    " + input_directory + "\n    " + output_directory);

    DirectoryLister directory_lister(true);

    for( const std::string& input_file_path : directory_lister.GetPaths(input_directory) )
    {
        ASSERT(SO::StartsWithNoCase(input_file_path, input_directory));

        CopyFile(input_file_path,
                 Path::Combine(output_directory, input_file_path.substr(input_directory.length())),
                 false);
    }
}


void Builder::BuildDocSet(const std::string& csdocset_file_path, const std::variant<const char*, BuildBlog> build_name_or_build_blog)
{
    const std::string csdocument_exe = Path::Combine(m_inputs.cspro_root, R"(cspro\build\x64\Debug\bin\CSDocument.exe)");

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

    const std::string ruby_exe = Path::Combine(m_inputs.ruby, "bin", "ruby.exe");
    const std::string jekyll_sh = Path::Combine(m_inputs.ruby, "bin", "jekyll");

    if( !PortableFunctions::FileIsRegular(ruby_exe) ||
        !PortableFunctions::FileIsRegular(jekyll_sh) )
    {
        throw CSProException("Ruby and Jekyll must exist at:\n%s\n%s", ruby_exe.c_str(), jekyll_sh.c_str());
    }

    const std::string site_output_directory = Path::Combine(m_inputs.csprousers_input, "_site");
    RecycleDirectory(site_output_directory);

    const std::string command = EscapeCommandLineArgument(ruby_exe)
                                .append(" ").append(EscapeCommandLineArgument(jekyll_sh))
                                .append(" build --config")
                                .append(" ").append(EscapeCommandLineArgument(Path::Combine(m_inputs.csprousers_input, "_config.yml")))
                                .append(",").append(EscapeCommandLineArgument(Path::Combine(m_inputs.csprousers_input, "_config_shared.yml")))
                                .append(",").append(EscapeCommandLineArgument(Path::Combine(m_inputs.csprousers_input, "_config_production.yml")));

    int return_code;

    if( !RunProgram(TC::ToWide(command), &return_code, SW_SHOWNA, true, true, TC::ToWide(m_inputs.csprousers_input).c_str()) ||
        !PortableFunctions::FileIsDirectory(site_output_directory) )
    {
        throw CSProException("Error running Jekyll: " + command);
    }

    ASSERT(return_code == 0);

    CopyDirectoryRecursive(site_output_directory, m_inputs.csprousers_output);
}


void Builder::UpdateBlog()
{
    m_loggingListBox.AddText("Building the blog...");

    const std::string posts_directory = Path::Combine(m_inputs.csprousers_input, "_posts");
    RecycleDirectory(posts_directory);

    const std::string csdocset_file_path = Path::Combine(m_inputs.csprousers_input, "blog", "CSPro Users Blog.csdocset");

    m_loggingListBox.AddText("Converting the blog posts in %s...", Path::GetFilename(csdocset_file_path).c_str());
    BuildDocSet(csdocset_file_path, BuildBlog { posts_directory });

    const std::vector<std::string> post_file_paths = DirectoryLister().GetPaths(posts_directory);

    if( !post_file_paths.empty() )
    {
        m_loggingListBox.AddText("Successfully converted %d blog post%s, the last one being:\n    %s",
                                 static_cast<int>(post_file_paths.size()), PluralizeWord(post_file_paths.size()),
                                 Path::GetFilename(post_file_paths.back()).c_str());
    }
}


void Builder::UpdateHelps()
{
    m_loggingListBox.AddText("Building the helps...");

    const std::string helps_output_directory = Path::Combine(m_inputs.csprousers_output, HelpDirectory_sv);

    // copy the resource files
    const std::string resource_files_json_file_path = Path::Combine(m_inputs.helps, "resource-files.json");
    m_loggingListBox.AddText("Copying resource files specified in %s...", resource_files_json_file_path.c_str());

    JsonReaderInterface json_reader_interface(m_inputs.helps);
    const JsonNode json_node = Json::ParseFile(resource_files_json_file_path, &json_reader_interface);

    for( const JsonNode& file_json_node : json_node.GetArray() )
    {
        const std::string resource_file_path = file_json_node.GetAbsolutePath();
        CopyFile(resource_file_path,
                 Path::Combine(helps_output_directory, "resources", Path::GetFilename(resource_file_path)));
    }

    const std::string csdocument_outputs_directory = Path::Combine(m_inputs.helps, "Outputs");
    RecycleDirectory(csdocument_outputs_directory);

    DirectoryLister directory_lister(true);
    directory_lister.SetNameFilter("*.csdocset");

    for( const std::string& csdocset_file_path : directory_lister.GetPaths(m_inputs.helps) )
    {
        // build the website
        m_loggingListBox.AddText("Building the website for %s...", Path::GetFilenameWithoutExtension(csdocset_file_path).c_str());
        BuildDocSet(csdocset_file_path, "CSPro Users Help Website");
    }

    CopyDirectoryRecursive(Path::Combine(csdocument_outputs_directory, "Website"),
                           helps_output_directory);
}


void Builder::UpdateMobileWorkshop()
{
    m_loggingListBox.AddText("Building the mobile workshop materials...");

    const std::string mobile_workshop_output_directory = Path::Combine(m_inputs.csprousers_output, MobileWorkshopDirectory_sv);

    const std::string csdocset_file_path = Path::Combine(m_inputs.mobile_workshop, "CSProMobileWorkshop", "CSProMobileWorkshop.csdocset");

    const std::string csdocument_outputs_directory = Path::Combine(m_inputs.mobile_workshop, "Outputs");
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
    directory_lister.AddPaths(zip_input_file_paths, Path::Combine(m_inputs.mobile_workshop, "FilesForExercises"));
    directory_lister.AddPaths(zip_input_file_paths, Path::Combine(m_inputs.mobile_workshop, "Questionnaire"));

    std::string zip_output_file_path = Path::Combine(mobile_workshop_output_directory, "materials", "cspro-mobile-workshop-materials.zip");
    FileIO::CreateDirectoriesForFile(zip_output_file_path);

    ZipCreator zip_creator(std::move(zip_output_file_path));
    zip_creator.AddFiles(zip_input_file_paths);
    zip_creator.Close();
}


void Builder::UpdateGooglePlayPrivacyPolicy()
{
    m_loggingListBox.AddText("Creating the Google Play privacy policy...");

    const std::string gcl_exe = Path::Combine(m_inputs.cspro_root, R"(build-tools\build\x64\Debug\bin\Generate Combined License.exe)");

    if( !PortableFunctions::FileIsRegular(gcl_exe) )
        throw CSProException("The Generate Combined License program must exist at: " + gcl_exe);

    const std::string privacy_path_directory = Path::Combine(m_inputs.csprousers_input, "privacy");
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


void Builder::ClearOutputs(const UINT nID)
{
    GitIgnoreEvaluator exclusion_evaluator;
    exclusion_evaluator.AddRules(m_inputs.csprousers_output_clear_exclusions);

    size_t files_deleted = 0;

    if( nID == IDC_CLEAR_SITE || nID == IDC_CLEAR_ALL )
    {
        // process files at the site root
        ClearOutputs(exclusion_evaluator, m_inputs.csprousers_output, false);

        // process each directory other than the helps and mobile workshop directories
        DirectoryLister directory_lister(false, false, true);

        for( std::string& directory_path : directory_lister.GetPaths(m_inputs.csprousers_output) )
        {
            Path::MakeRemoveTrailingSlash(directory_path);
            const std::string directory_name = Path::GetFilename(directory_path);

            if( directory_name != HelpDirectory_sv &&
                directory_name != MobileWorkshopDirectory_sv )
            {
                files_deleted += ClearOutputs(exclusion_evaluator, directory_path, true);
            }
        }
    }

    if( nID == IDC_CLEAR_HELPS || nID == IDC_CLEAR_ALL )
    {
        files_deleted += ClearOutputs(
            exclusion_evaluator,
            Path::Combine(m_inputs.csprousers_output, HelpDirectory_sv),
            true
        );
    }

    if( nID == IDC_CLEAR_MOBILE_WORKSHOP || nID == IDC_CLEAR_ALL )
    {
        files_deleted += ClearOutputs(
            exclusion_evaluator,
            Path::Combine(m_inputs.csprousers_output, MobileWorkshopDirectory_sv),
            true
        );
    }

    m_loggingListBox.AddText("Files deleted: %zu", files_deleted);
}


size_t Builder::ClearOutputs(GitIgnoreEvaluator& exclusion_evaluator, const std::string& directory_path, const bool recursive)
{
    m_loggingListBox.AddText(
        "Clearing outputs (recursive = %s) in: %s)",
        recursive ? "true" : "false",
        directory_path.c_str()
    );

    DirectoryLister directory_lister(recursive);
    size_t files_deleted = 0;

    size_t output_path_prefix_to_clear = m_inputs.csprousers_output.length();

    if( !Path::IsSlashChar(m_inputs.csprousers_output.back()) )
        ++output_path_prefix_to_clear;

    for( const std::string& file_path : directory_lister.GetPaths(directory_path) )
    {
        // exclusions paths are based off the output directory
        ASSERT(SO::StartsWith(file_path, m_inputs.csprousers_output));
        std::string repository_style_path = file_path.substr(output_path_prefix_to_clear);
        Path::MakeToForwardSlash(repository_style_path);

        if( exclusion_evaluator.Include(repository_style_path) )
        {
            PortableFunctions::FileDeleteWithExceptions(file_path);
            ++files_deleted;
        }
    }

    return files_deleted;
}


void Builder::CreateWebsiteUpdaters(const std::string& last_processed_commit_sha)
{
    // make sure that the built website is part of the repository
    if( !SO::StartsWith(m_inputs.csprousers_output, m_inputs.csprousers_files_repository) )
        throw CSProException("The built website directory cannot be outside the repository: " + m_inputs.csprousers_output);

    std::string website_repository_path =
        PortableFunctions::PathEnsureTrailingForwardSlash(
            Path::ToForwardSlash(
                m_inputs.csprousers_output.substr(m_inputs.csprousers_files_repository.length())
            )
        );
    SO::MakeTrimLeft(website_repository_path, '/');

    const std::string git_directory = Path::Combine(m_inputs.csprousers_files_repository, ".git");
    m_loggingListBox.AddText("Opening repository: " + git_directory);

    GitRepository repo;
    repo.OpenBare(git_directory);

    // process the differences between the current and last processed commits
    const GitCommit last_processed_commit = repo.LookupCommit(last_processed_commit_sha);
    GitTree last_processed_tree = last_processed_commit.GetTree();

    const GitCommit current_commit = repo.LookupCommit(repo.GetCurrentBranch());
    GitTree current_tree = current_commit.GetTree();

    std::vector<std::tuple<std::string, GitObjectId>> added_modified_files;
    std::vector<std::string> removed_files;

    const GitDiff diff = repo.GetDifference(last_processed_tree, current_tree);

    diff.ForeachDifference(
        [&](const void* const delta)
        {
            const git_diff_delta* const diff_delta = static_cast<const git_diff_delta*>(delta);
            std::string path = diff_delta->new_file.path;

            if( !SO::StartsWithNoCase(path, website_repository_path) )
            {
                m_loggingListBox.AddText("Ignoring: " + path);
                return true;
            }

            std::string website_file_path = path.substr(website_repository_path.length());
            ASSERT(!Path::IsSlashChar(website_file_path.front()));

            if( diff_delta->status == GIT_DELTA_ADDED ||
                diff_delta->status == GIT_DELTA_MODIFIED )
            {
                m_loggingListBox.AddText("%s: %s",
                    ( diff_delta->status == GIT_DELTA_ADDED ) ? "Adding" : "Modifying",
                    website_file_path.c_str()
                );

                added_modified_files.emplace_back(std::move(website_file_path), diff_delta->new_file.id);
            }

            else if( diff_delta->status == GIT_DELTA_DELETED )
            {
                m_loggingListBox.AddText("Deleting: " + website_file_path);
                removed_files.emplace_back(std::move(website_file_path));
            }

            else
            {
                throw CSProException("Unknown diff status: '%s' -> %d", path.c_str(), static_cast<int>(diff_delta->status));
            }

            return true;
        });

    if( added_modified_files.empty() && removed_files.empty() )
    {
        m_loggingListBox.AddText("No changes since the last processed commit.");
        return;
    }

    // create a directory to store the updaters
    const std::string updaters_directory = Path::Combine(
        m_inputs.csprousers_output,
        IntToString(GetTimestamp()) + "-updater"
    );

    FileIO::CreateDirectories(updaters_directory);

    // create a script to remove files
    if( !removed_files.empty() )
    {
        const std::string& script_file_path = Path::Combine(updaters_directory, "remove-files.sh");

        m_loggingListBox.AddText("Creating a removal script for %zu file%s: %s: ",
            removed_files.size(), PluralizeWord(removed_files.size()),
            script_file_path.c_str()
        );

        CreateWebsiteRemoveScript(script_file_path, removed_files);
    }

    // create a ZIP file with added and modified files
    if( !added_modified_files.empty() )
    {
        const std::string& zip_file_path = Path::Combine(updaters_directory, "files.zip");

        m_loggingListBox.AddText("Creating a ZIP file for %zu file%s: %s: ",
            added_modified_files.size(), PluralizeWord(added_modified_files.size()),
            zip_file_path.c_str()
        );

        CreateWebsiteFilesZip(zip_file_path, repo, added_modified_files);
    }

    // show the updaters
    OpenContainingFolder(updaters_directory);
}


void Builder::CreateWebsiteRemoveScript(const std::string& script_file_path, const std::vector<std::string>& removed_files)
{
    FileIO::TextFile text_file;

    static_assert(TextEncoding::DefaultEncoding != TextEncoding::Type::Utf8);
    text_file.SetTextEncoding(TextEncoding::Type::Utf8);

    static_assert(FileIO::TextFile::DefaultWriteNewlineAsCRLF);
    text_file.SetWriteNewlineAsCRLF(false);

    text_file.OpenForTextWritingCreate(script_file_path);

    text_file.WriteLine("#!/bin/sh");

    for( const std::string& removed_file : removed_files )
        text_file.WriteFormattedLine("rm '%s'", removed_file.c_str());

    text_file.Close();
}


void Builder::CreateWebsiteFilesZip(const std::string& zip_file_path, GitRepository& repo,
                                    const std::vector<std::tuple<std::string, GitObjectId>>& added_modified_files)
{
    ZipCreator zip_creator(zip_file_path);

    for( const auto& [file_path, oid] : added_modified_files )
    {
        const GitBlob blob = repo.LookupBlob(oid);
        zip_creator.AddContent(file_path, blob.data(), blob.size());
    }

    zip_creator.Close();
}
